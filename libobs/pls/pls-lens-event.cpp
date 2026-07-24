#include "pls-lens-event.h"
#include "task-pool.h"
#include "pls-base.h"
#include <map>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#ifdef _WIN32
#include <Windows.h>
#include <string>
#include "win/prism-ipc-impl.h"
#include "win/handle-wrapper.h"
#include "win/monitor-lens-install.h"
#elif defined(__APPLE__)
#include "mac/prism-ipc-impl.h"
// Defined in pls-lens-event-mac.m
enum PLSLensEventType {
	PLS_LENS_EVENT_ACTIVE = 0,
	PLS_LENS_EVENT_LICENSE,
	PLS_LENS_EVENT_VB,
	PLS_LENS_EVENT_CAPTURE,
	PLS_LENS_EVENT_ONOFF,
	PLS_LENS_EVENT_UI_CONTROLLABLE,
	PLS_LENS_EVENT_COUNT
};
typedef void (*pls_lens_event_cb_t)(int lens_index, int event_type, uint64_t state, void *context);
extern "C" void pls_register_all_lens_notify(int lens_index, pls_lens_event_cb_t cb, void *context, int *tokens_out);
extern "C" void pls_unregister_all_lens_notify(int *tokens, int count);
extern "C" bool pls_get_lens_state_from_token(int token, uint64_t *stateOut);
extern "C" bool pls_is_lens_app_running(void);
extern "C" bool pls_is_lens_installed(void);
extern "C" bool pls_is_lens_state_supported(void);
#endif

#define CHECK_LENS_INVALID(cam_index, handle)                    \
	if (cam_index < 0 || cam_index >= (int)MAX_LENS_COUNT) { \
		assert(false);                                   \
		handle;                                          \
	}

class PLSLensEventCenter {
	PLSLensEventCenter();

public:
	~PLSLensEventCenter();

	static PLSLensEventCenter *Get()
	{
		static PLSLensEventCenter instance;
		return &instance;
	}

	void StartMonitor();
	void StopMonitor();

	void RegisterEvents(void *receiver, const LensEvents &cbs);
	void UnregisterEvents(void *receiver);

	void EnumHandler(std::function<void(std::shared_ptr<LensEvents>)> cb);

protected:
	void MonitorThread();
	// {force_notify}: It should be "true" if install state of lens is changed
	void UpdateDeviceState(int index, bool is_active, bool force_notify = false);
	void UpdateCaptureState(int index, bool is_ready, bool force_notify = false);
	void UpdateOnOffState(int index, bool is_on, bool force_notify = false);
	void UpdateUiControllableState(bool is_ui_controllable, bool force_notify = false);

	bool AnyLensReady() const
	{
		for (int i = 0; i < MAX_LENS_COUNT; i++) {
			if (onoff_stats[i].load() && lens_stats[i].load())
				return true;
		}
		return false;
	}

#if defined(__APPLE__)
	// Reset all lens states to defaults (false) and notify listeners.
	// Should be called when Lens is uninstalled or not running after reinstall.
	void ResetAllLensStates(bool force_notify);
#endif

private:
	std::recursive_mutex mutex_cbs;
	std::map<void *, std::shared_ptr<LensEvents>> lens_event_cbs;

	// Monitor thread members (shared by both platforms)
	std::mutex exit_mutex;
	std::atomic<bool> exit_flag = false;
	std::condition_variable exit_cond;
	std::shared_ptr<std::thread> monitor_thread = nullptr;

#if defined(__APPLE__)
	std::atomic<bool> mac_observing = false;
	int mac_tokens[MAX_LENS_COUNT][PLS_LENS_EVENT_COUNT] = {};
#endif
public:
	std::shared_ptr<ITaskPool> async_task = nullptr;

#if defined(__APPLE__)
	std::atomic<bool> is_lens_ui_controllable = false;
#endif
	std::atomic<bool> is_lens_running = false;
	std::atomic<bool> is_lens_license_verified = false;
	std::atomic<bool> lens_stats[MAX_LENS_COUNT] = {false, false, false};
	std::atomic<bool> capture_stats[MAX_LENS_COUNT] = {false, false, false};
	std::atomic<bool> onoff_stats[MAX_LENS_COUNT] = {false, false, false};
	std::atomic<bool> lens_vb_removed[MAX_LENS_COUNT] = {false, false, false};
#if defined(_WIN32) || defined(__APPLE__)
	std::shared_ptr<IPCPrismLens> lens_ipc[MAX_LENS_COUNT] = {};
	void CheckLicense(std::shared_ptr<IPCPrismLens> ipc);
	void CheckVbState(std::shared_ptr<IPCPrismLens> ipc, int index);
#endif

#ifdef _WIN32
	HANDLE resolution_changed_evts[MAX_LENS_COUNT] = {};
	void CheckResolution(int index);
#endif
};

//----------------------------------------------------------------------------------------------------
PLSLensEventCenter::PLSLensEventCenter()
{
	async_task = create_task_pool();

	for (size_t i = 0; i < MAX_LENS_COUNT; i++) {
		lens_stats[i] = false;
		capture_stats[i] = false;
		lens_vb_removed[i] = false;
#if defined(_WIN32) || defined(__APPLE__)
		lens_ipc[i] = std::make_shared<IPCPrismLens>(static_cast<int>(i));
		lens_ipc[i]->init_ipc_sessions();
#endif

#ifdef _WIN32
		auto eventName = std::string("output-resolution-changed") + "-" + std::to_string(i);
		resolution_changed_evts[i] = CHandleWrapper::GetEvent(eventName.c_str(), true);
#endif
	}
}

PLSLensEventCenter::~PLSLensEventCenter()
{
	StopMonitor();

#ifdef _WIN32
	for (size_t i = 0; i < MAX_LENS_COUNT; i++) {
		CHandleWrapper::CloseHandleEx(resolution_changed_evts[i]);
	}
#endif
}

void PLSLensEventCenter::StartMonitor()
{
#if defined(__APPLE__)
	assert(!mac_observing.load());
	mac_observing = true;

	// The UI controllable token is published by Cam Studio on index 0 only.
	// We read it as an app-level gate, then combine with AnyLensReady()
	// (any lens on+active) so that disabling one lens doesn't block others.
	constexpr int kUiControllableIndex = 0;

	// Single unified callback for all event types
	auto event_cb = [](int lens_index, int event_type, uint64_t state, void *ctx) {
		auto *self = static_cast<PLSLensEventCenter *>(ctx);
		if (!self || !self->mac_observing.load())
			return;

		// Ignore notify callbacks when Lens is not running.
		// This prevents stale notifyd state from overwriting reset values
		// after uninstall/reinstall when Lens hasn't launched yet.
		if (!self->is_lens_running.load())
			return;

		switch (event_type) {
		case PLS_LENS_EVENT_ACTIVE:
			if (lens_index >= 0 && lens_index < MAX_LENS_COUNT) {
				const bool prev_active = self->lens_stats[lens_index].load();
				const bool next_active = (state != 0);
				if (prev_active != next_active) {
					blog(LOG_INFO, "LensEventNotify: active notify lens[%d] %d->%d", lens_index,
					     prev_active ? 1 : 0, next_active ? 1 : 0);
				}
			}
			self->UpdateDeviceState(lens_index, state != 0);
			break;
		case PLS_LENS_EVENT_LICENSE:
			if (auto ipc = self->lens_ipc[lens_index])
				self->CheckLicense(ipc);
			break;
		case PLS_LENS_EVENT_VB:
			if (auto ipc = self->lens_ipc[lens_index])
				self->CheckVbState(ipc, lens_index);
			break;
		case PLS_LENS_EVENT_CAPTURE:
			if (auto ipc = self->lens_ipc[lens_index])
				self->UpdateCaptureState(lens_index, ipc->is_capture_valid());
			break;
		case PLS_LENS_EVENT_ONOFF:
			if (auto ipc = self->lens_ipc[lens_index])
				self->UpdateOnOffState(lens_index, ipc->is_lens_on());
			break;
		case PLS_LENS_EVENT_UI_CONTROLLABLE:
			if (lens_index == kUiControllableIndex) {
				const bool ui_controllable = (state != 0) && self->is_lens_license_verified.load() && self->AnyLensReady();
				self->UpdateUiControllableState(ui_controllable);
			}
			break;
		}
	};

	// Check if Lens is currently running. We only seed initial state from
	// notify/IPC if Lens is running, otherwise we keep defaults (false) to
	// avoid showing stale notifyd values after uninstall/reinstall.
	bool lens_running = pls_is_lens_app_running();
	is_lens_running = lens_running;

	for (int i = 0; i < MAX_LENS_COUNT; ++i) {
		// Register all event channels for this lens index
		pls_register_all_lens_notify(i, event_cb, this, mac_tokens[i]);

		if (lens_running) {
			// Initial state read for all event types (so UI has correct state without waiting for events)
			auto ipc = lens_ipc[i];
			if (ipc) {
				UpdateCaptureState(i, ipc->is_capture_valid());
				UpdateOnOffState(i, ipc->is_lens_on());
				CheckVbState(ipc, i);
				CheckLicense(ipc);
			}

			// Active state
			uint64_t state = 0;
			if (pls_get_lens_state_from_token(mac_tokens[i][PLS_LENS_EVENT_ACTIVE], &state)) {
				UpdateDeviceState(i, state != 0);
			} else {
				UpdateDeviceState(i, false);
			}
		} else {
			// Lens not running, keep all states at defaults (false)
			UpdateDeviceState(i, false);
			UpdateCaptureState(i, false);
			UpdateOnOffState(i, false);
		}
	}

	bool ui_controllable = false;
	if (lens_running) {
		uint64_t ui_state = 0;
		if (pls_get_lens_state_from_token(mac_tokens[kUiControllableIndex][PLS_LENS_EVENT_UI_CONTROLLABLE], &ui_state)) {
			ui_controllable = (ui_state != 0);
		}
		ui_controllable = ui_controllable && is_lens_license_verified.load() && AnyLensReady();
	}
	UpdateUiControllableState(ui_controllable, true);

	// Start monitor thread to periodically check lens installation state
	assert(monitor_thread == nullptr);
	exit_flag = false;
	monitor_thread = std::make_shared<std::thread>(&PLSLensEventCenter::MonitorThread, this);
#else
	assert(monitor_thread == nullptr);
	monitor_thread = std::make_shared<std::thread>(&PLSLensEventCenter::MonitorThread, this);
#endif
}

void PLSLensEventCenter::StopMonitor()
{
#if defined(__APPLE__)
	if (!mac_observing.load())
		return;

	mac_observing = false;

	// Stop monitor thread first
	{
		std::unique_lock<std::mutex> lock(exit_mutex);
		exit_flag = true;
		exit_cond.notify_all();
	}

	if (monitor_thread && monitor_thread->joinable()) {
		monitor_thread->join();
	}
	monitor_thread = nullptr;

	for (int i = 0; i < MAX_LENS_COUNT; ++i) {
		pls_unregister_all_lens_notify(mac_tokens[i], PLS_LENS_EVENT_COUNT);
	}
#else
	if (!monitor_thread)
		return;

	{
		std::unique_lock<std::mutex> lock(exit_mutex);
		exit_flag = true;
		exit_cond.notify_all();
	}

	if (monitor_thread->joinable()) {
		monitor_thread->join();
	}
	monitor_thread = nullptr;
#endif
}

void PLSLensEventCenter::RegisterEvents(void *receiver, const LensEvents &cbs)
{
	if (!receiver)
		return;

	std::lock_guard<std::recursive_mutex> lock(mutex_cbs);
	auto itr = lens_event_cbs.find(receiver);
	if (itr != lens_event_cbs.end()) {
		assert(false && "repeat register ?");
	}
	lens_event_cbs[receiver] = std::make_shared<LensEvents>(cbs);
}

void PLSLensEventCenter::UnregisterEvents(void *receiver)
{
	if (!receiver)
		return;

	std::lock_guard<std::recursive_mutex> lock(mutex_cbs);
	auto itr = lens_event_cbs.find(receiver);
	if (itr != lens_event_cbs.end())
		lens_event_cbs.erase(itr);
}

void PLSLensEventCenter::EnumHandler(std::function<void(std::shared_ptr<LensEvents>)> cb)
{
	if (!cb)
		return;

	std::lock_guard<std::recursive_mutex> lock(mutex_cbs);
	for (auto &pair : lens_event_cbs) {
		if (pair.second)
			cb(pair.second);
	}
}

void PLSLensEventCenter::CheckLicense(std::shared_ptr<IPCPrismLens> ipc)
{
#ifdef _WIN32
	bool verified = IPCPrismLens::is_license_verified();
#else
	if (!ipc) {
		return;
	}
	auto verified = ipc->is_license_verified();
#endif

	if (verified != is_lens_license_verified) {
		is_lens_license_verified = verified;

		EnumHandler([verified](std::shared_ptr<LensEvents> cbs) {
			if (cbs && cbs->license_verified_cb) {
				cbs->license_verified_cb();
			}
		});
	}
}

void PLSLensEventCenter::CheckVbState(std::shared_ptr<IPCPrismLens> ipc, int cam_index)
{
	CHECK_LENS_INVALID(cam_index, return);

	if (!ipc)
		return;

	auto vb_removed = ipc->is_vb_removed();
	auto &cached_vb_removed = lens_vb_removed[cam_index];

	if (cached_vb_removed != vb_removed) {
		cached_vb_removed = vb_removed;

		EnumHandler([cam_index, vb_removed](std::shared_ptr<LensEvents> cbs) {
			if (cbs && cbs->vb_state_vb) {
				cbs->vb_state_vb(cam_index, vb_removed);
			}
		});
	}
}

#ifdef _WIN32
void PLSLensEventCenter::CheckResolution(int cam_index)
{
	if (cam_index < 0 || cam_index >= MAX_LENS_COUNT)
		return;

	const auto &handle = resolution_changed_evts[cam_index];
	if (CHandleWrapper::IsHandleValid(handle) && CHandleWrapper::IsHandleSigned(handle)) {
		::ResetEvent(handle);
		pls_check_lens_resolution(cam_index);
	}
}

bool IsLensActived(std::shared_ptr<IPCPrismLens> ipc, PLSMonitorLensInstall &monitor_lens_install)
{
	if (!ipc)
		return false;

	if (ipc->is_lens_actived())
		return true;

	win_version_info version = {};
	ipc->get_lens_version(version.major, version.minor, version.build);
	if (version.major > 0) {
		// get lens version from ipc, avoid reading lens exe
		if (!PLSMonitorLensInstall::IsLensSupportActive(version))
			return true;

	} else {
		// get lens version from reading file
		if (!monitor_lens_install.IsLensSupportActive())
			return true;
	}

	return false;
}

void PLSLensEventCenter::MonitorThread()
{
	auto pre_time = std::chrono::steady_clock::now();
	const auto interval = std::chrono::milliseconds(100);

	PLSMonitorLensInstall monitor_lens_install;

	while (false == exit_flag) {
		{
			std::unique_lock<std::mutex> lock(exit_mutex);
			exit_cond.wait_for(lock, std::chrono::milliseconds(10), [this] {
				return exit_flag.load(); // return true to break wait soon
			});

			if (exit_flag)
				break;
		}

		async_task->run_all_tasks();

		auto current_time = std::chrono::steady_clock::now();
		if ((current_time - pre_time) < interval)
			continue;

		bool lens_running = IPCPrismLens::is_handle_existed(LENS_RUNNING_FLAG);
		if (lens_running != is_lens_running) {
			is_lens_running = lens_running;

			EnumHandler([lens_running](std::shared_ptr<LensEvents> cbs) {
				if (cbs && cbs->lens_running_cb) {
					cbs->lens_running_cb(lens_running);
				}
			});
		}

		bool install_changed = false;
		monitor_lens_install.IsLensInstalled(install_changed);

		for (int i = 0; i < MAX_LENS_COUNT; i++) {
			auto ipc = lens_ipc[i];
			if (ipc) {
				ipc->tick();

				CheckResolution(i);

				CheckVbState(ipc, i);

				UpdateDeviceState(i, IsLensActived(ipc, monitor_lens_install), install_changed);

				UpdateCaptureState(i, ipc->is_capture_valid(), install_changed);

				UpdateOnOffState(i, ipc->is_lens_on(), install_changed);
			}
		}

		CheckLicense(nullptr);
		pre_time = current_time;
	}

	async_task->run_all_tasks();
}
#elif defined(__APPLE__)
void PLSLensEventCenter::ResetAllLensStates(bool force_notify)
{
	// Reset all lens states to defaults (false). Useful when Lens is uninstalled
	// or reinstalled but not yet running, to avoid reporting stale notifyd values.

	for (int i = 0; i < MAX_LENS_COUNT; i++) {
		UpdateDeviceState(i, false, force_notify);
		UpdateCaptureState(i, false, force_notify);
		UpdateOnOffState(i, false, force_notify);

		// Reset VB removed state
		if (lens_vb_removed[i].load() != false || force_notify) {
			lens_vb_removed[i] = false;
			EnumHandler([i](std::shared_ptr<LensEvents> cbs) {
				if (cbs && cbs->vb_state_vb) {
					cbs->vb_state_vb(i, false);
				}
			});
		}
	}

	UpdateUiControllableState(false, force_notify);

	// Reset license verified state
	if (is_lens_license_verified != false) {
		is_lens_license_verified = false;
		EnumHandler([](std::shared_ptr<LensEvents> cbs) {
			if (cbs && cbs->license_verified_cb) {
				cbs->license_verified_cb();
			}
		});
	}

	blog(LOG_INFO, "%s: All lens states reset to defaults", __FUNCTION__);
}

void PLSLensEventCenter::MonitorThread()
{
	auto pre_time = std::chrono::steady_clock::now();
	const auto check_interval = std::chrono::milliseconds(100);
	const auto install_check_interval = std::chrono::milliseconds(500);
	constexpr int kUiControllableIndex = 0;
	const auto reconcile_check_interval = std::chrono::milliseconds(500);
	auto last_install_check_time = std::chrono::steady_clock::now();
	auto last_reconcile_check_time = std::chrono::steady_clock::now();
	bool cached_lens_installed = pls_is_lens_installed();
	bool token_read_error_seen[MAX_LENS_COUNT] = {};

	blog(LOG_INFO, "%s lens installed: %s", __FUNCTION__, cached_lens_installed ? "yes" : "no");

	while (false == exit_flag) {
		{
			std::unique_lock<std::mutex> lock(exit_mutex);
			exit_cond.wait_for(lock, std::chrono::milliseconds(10), [this] { return exit_flag.load(); });

			if (exit_flag)
				break;
		}

//		async_task->run_all_tasks();

		auto current_time = std::chrono::steady_clock::now();
		if ((current_time - pre_time) < check_interval)
			continue;

		// Check lens installation state periodically (every 500ms)
		bool install_changed = false;
		if ((current_time - last_install_check_time) >= install_check_interval) {
			bool lens_installed = pls_is_lens_installed();
			if (lens_installed != cached_lens_installed) {
				install_changed = true;
				cached_lens_installed = lens_installed;
				blog(LOG_INFO, "%s lens installed: %s", __FUNCTION__, lens_installed ? "yes" : "no");
			}
			last_install_check_time = current_time;
		}

		// If installation state changed, reset all lens states to defaults.
		// This handles both uninstall and reinstall - we reset because:
		// - On uninstall: Lens is gone, states should be false
		// - On reinstall: New Lens hasn't launched yet, states should be false
		//   until the new instance publishes fresh values via notify callbacks.
		if (install_changed) {
			ResetAllLensStates(true);
		}

		// Check lens running state periodically
		bool lens_running = pls_is_lens_app_running();
		if (lens_running != is_lens_running) {
			bool was_running = is_lens_running.load();
			is_lens_running = lens_running;

			// Notify running state change
			EnumHandler([lens_running](std::shared_ptr<LensEvents> cbs) {
				if (cbs && cbs->lens_running_cb) {
					cbs->lens_running_cb(lens_running);
				}
			});

			if (!was_running && lens_running) {
				// Lens just started running - refresh state from notify tokens.
				// This handles the case where Lens was reinstalled and now launched.
				blog(LOG_INFO, "%s: Lens started running, refreshing state from notify tokens", __FUNCTION__);
				uint64_t ui_state = 0;
				bool ui_controllable_from_token = false;
				if (pls_get_lens_state_from_token(mac_tokens[kUiControllableIndex][PLS_LENS_EVENT_UI_CONTROLLABLE], &ui_state)) {
					ui_controllable_from_token = (ui_state != 0);
				}

				for (int i = 0; i < MAX_LENS_COUNT; i++) {
					auto ipc = lens_ipc[i];
					if (ipc) {
						UpdateDeviceState(i, false, true);
						UpdateCaptureState(i, ipc->is_capture_valid(), true);
						UpdateOnOffState(i, ipc->is_lens_on(), true);
						CheckVbState(ipc, i);
						CheckLicense(ipc);
					}
				}
			bool ui_controllable = ui_controllable_from_token && is_lens_license_verified.load() && AnyLensReady();
			UpdateUiControllableState(ui_controllable, true);
			} else if (was_running && !lens_running) {
				blog(LOG_INFO, "%s: Lens stopped running, forcing all states inactive", __FUNCTION__);
				// When lens stops running, mark all devices inactive
				for (int i = 0; i < MAX_LENS_COUNT; i++) {
					UpdateDeviceState(i, false, true);
					UpdateCaptureState(i, false, true);
					UpdateOnOffState(i, false, true);
					UpdateUiControllableState(false, true);
				}
			}
		}

		// Reconcile states periodically while Lens is running.
		// On macOS notify callbacks can occasionally be delayed/missed, so polling
		// prevents stale token values from persisting in UI.
		if (lens_running && (current_time - last_reconcile_check_time) >= reconcile_check_interval) {
			for (int i = 0; i < MAX_LENS_COUNT; i++) {
				auto ipc = lens_ipc[i];
				if (!ipc) {
					continue;
				}

				const bool prev_active = lens_stats[i];
				const bool prev_capture = capture_stats[i];
				const bool prev_onoff = onoff_stats[i];

				uint64_t token_state = 0;
				const bool token_ok =
					pls_get_lens_state_from_token(mac_tokens[i][PLS_LENS_EVENT_ACTIVE], &token_state);
				const bool active = token_ok ? (token_state != 0) : false;
				const bool capture_ready = ipc->is_capture_valid();
				const bool lens_on = ipc->is_lens_on();

				UpdateDeviceState(i, active);
				UpdateCaptureState(i, capture_ready);
				UpdateOnOffState(i, lens_on);
				CheckVbState(ipc, i);
				CheckLicense(ipc);

				const bool curr_active = lens_stats[i];
				const bool curr_capture = capture_stats[i];
				const bool curr_onoff = onoff_stats[i];
				if (!token_ok) {
					if (!token_read_error_seen[i]) {
						token_read_error_seen[i] = true;
						blog(LOG_WARNING,
						     "%s: reconcile lens[%d] failed to read active token; using inactive fallback",
						     __FUNCTION__, i);
					}
				} else if (token_read_error_seen[i]) {
					token_read_error_seen[i] = false;
					blog(LOG_INFO,
					     "%s: reconcile lens[%d] active token read recovered (token_state=%llu)",
					     __FUNCTION__, i, static_cast<unsigned long long>(token_state));
				}

				if (prev_active != curr_active || prev_capture != curr_capture ||
				    prev_onoff != curr_onoff) {
					blog(LOG_INFO,
					     "%s: reconcile lens[%d] token_state=%llu active %d->%d capture %d->%d onoff %d->%d",
					     __FUNCTION__, i,
					     static_cast<unsigned long long>(token_state), prev_active ? 1 : 0,
					     curr_active ? 1 : 0, prev_capture ? 1 : 0, curr_capture ? 1 : 0,
					     prev_onoff ? 1 : 0, curr_onoff ? 1 : 0);
				}
			}

			uint64_t ui_state = 0;
			bool ui_controllable_from_token = false;
			if (pls_get_lens_state_from_token(mac_tokens[kUiControllableIndex][PLS_LENS_EVENT_UI_CONTROLLABLE], &ui_state)) {
				ui_controllable_from_token = (ui_state != 0);
			}
			bool ui_controllable = ui_controllable_from_token && is_lens_license_verified.load() && AnyLensReady();
			UpdateUiControllableState(ui_controllable);

			last_reconcile_check_time = current_time;
		}

		pre_time = current_time;
	}

	async_task->run_all_tasks();
}
#endif

void PLSLensEventCenter::UpdateDeviceState(int index, bool is_active, bool force_notify)
{
	CHECK_LENS_INVALID(index, return);

	bool previous_state = lens_stats[index];
	bool final_state = is_active;
#if defined(__APPLE__)
	if (!pls_is_lens_state_supported()) {
		final_state = true;
	}
#endif
	if (previous_state != final_state || force_notify) {
		lens_stats[index] = final_state; // firstly update the status, then notify events

		EnumHandler([index, final_state](std::shared_ptr<LensEvents> cbs) {
			if (cbs && cbs->active_cb) {
				cbs->active_cb(index, final_state);
			}
		});
	}
}

void PLSLensEventCenter::UpdateCaptureState(int index, bool is_ready, bool force_notify)
{
	bool previous_state = capture_stats[index];
	if (previous_state != is_ready || force_notify) {
		capture_stats[index] = is_ready; // firstly update the status, then notify events

		EnumHandler([index, is_ready](std::shared_ptr<LensEvents> cbs) {
			if (cbs && cbs->capture_cb) {
				cbs->capture_cb(index, is_ready);
			}
		});
	}
}

void PLSLensEventCenter::UpdateOnOffState(int index, bool is_on, bool force_notify)
{
	bool previous_state = onoff_stats[index];
	if (previous_state != is_on || force_notify) {
		onoff_stats[index] = is_on; // firstly update the status, then notify events

		EnumHandler([index, is_on](std::shared_ptr<LensEvents> cbs) {
			if (cbs && cbs->onoff_cb) {
				cbs->onoff_cb(index, is_on);
			}
		});
	}
}

#if defined(__APPLE__)
void PLSLensEventCenter::UpdateUiControllableState(bool is_ui_controllable, bool force_notify)
{
	bool previous_state = is_lens_ui_controllable;
	if (previous_state != is_ui_controllable || force_notify) {
		is_lens_ui_controllable = is_ui_controllable;
	}
}
#endif
//----------------------------------------------------------------------------------------------------
void pls_start_monitor_lens_events()
{
	PLSLensEventCenter::Get()->StartMonitor();
}

void pls_stop_monitor_lens_events()
{
	PLSLensEventCenter::Get()->StopMonitor();
}

void pls_register_lens_events(void *receiver, const LensEvents &cbs)
{
	PLSLensEventCenter::Get()->RegisterEvents(receiver, cbs);
}

void pls_unregister_lens_events(void *receiver)
{
	PLSLensEventCenter::Get()->UnregisterEvents(receiver);
}

bool pls_is_lens_active(int cam_index)
{
	CHECK_LENS_INVALID(cam_index, return false);
	return PLSLensEventCenter::Get()->lens_stats[cam_index];
}

bool pls_is_lens_capture_ready(int cam_index)
{
	CHECK_LENS_INVALID(cam_index, return false);
	return PLSLensEventCenter::Get()->capture_stats[cam_index];
}

bool pls_is_lens_on(int cam_index)
{
	CHECK_LENS_INVALID(cam_index, return false);
	return PLSLensEventCenter::Get()->onoff_stats[cam_index];
}

bool pls_is_lens_running()
{
#if defined(__APPLE__)
	return pls_is_lens_app_running();
#else
	return PLSLensEventCenter::Get()->is_lens_running;
#endif
}

bool pls_is_lens_license_verified()
{
	return PLSLensEventCenter::Get()->is_lens_license_verified;
}

bool pls_is_lens_ui_controllable()
{
#if defined(__APPLE__)
	return PLSLensEventCenter::Get()->is_lens_ui_controllable;
#else
	return IPCPrismLens::is_lens_ui_controllable();
#endif
}

bool pls_is_lens_vb_removed(int cam_index)
{
	CHECK_LENS_INVALID(cam_index, return false);
	return PLSLensEventCenter::Get()->lens_vb_removed[cam_index];
}

void pls_request_lens_remove_vb(int cam_index, bool need_remove)
{
	CHECK_LENS_INVALID(cam_index, return);
#if defined(__APPLE__)
	auto ipc = PLSLensEventCenter::Get()->lens_ipc[cam_index];
	if (ipc) {
		ipc->request_lens_remove_vb(need_remove);
	}
#else
	PLSLensEventCenter::Get()->async_task->push_task([cam_index, need_remove]() {
		auto ipc = PLSLensEventCenter::Get()->lens_ipc[cam_index];
		if (ipc) {
			ipc->request_lens_remove_vb(need_remove);
			if (need_remove) {
				PLS_UI_ACTION_OBS("sent vb removed");
			} else {
				PLS_UI_ACTION_OBS("sent vb original");
			}
		}
	});
#endif
}
