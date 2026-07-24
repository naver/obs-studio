#include "prism-ipc-impl.h"
#include "handle-wrapper.h"
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <PathCch.h>

#define FILEMAP_LENS_LICENSE_EVENT "lens-license-verified-event" // whether lens license is verified
#define FILEMAP_LENS_UI_EVENT "lens-ui-controllable-event"

std::mutex IPCPrismLens::exit_callback_mutex;
std::vector<IExitCallback *> IPCPrismLens::exit_callbacks;

std::atomic<int> event_ref = 0;
std::shared_ptr<CSignalEvent> license_flag = nullptr;
std::shared_ptr<CSignalEvent> ui_controllable_flag = nullptr;

//------------------------------------------------------------------------------------------------
bool IPCPrismLens::is_lens_app()
{
	static bool is_inited = false;
	static bool is_lens = false;

	if (is_inited)
		return is_lens;

	WCHAR path[MAX_PATH] = {};
	GetModuleFileNameW(NULL, path, MAX_PATH);

	std::wstring exe_name = path;
	LPTSTR temp = _tcsrchr(path, _T('\\'));
	if (temp != NULL) {
		exe_name = temp + 1;
	}

	is_inited = true;
	is_lens = (wcsstr(exe_name.c_str(), L"PRISMLens") != NULL);

	return is_lens;
}

bool IPCPrismLens::is_handle_existed(const wchar_t *name)
{
	if (!name) {
		return false;
	}

	HANDLE handle = OpenEventW(EVENT_ALL_ACCESS, false, name);
	if (!handle || handle == INVALID_HANDLE_VALUE) {
		return false;
	}
	CloseHandle(handle);
	return true;
}

void init_license_verified_event()
{
	++event_ref;
	if (event_ref == 1) {
		license_flag = std::make_shared<CSignalEvent>(FILEMAP_LENS_LICENSE_EVENT);
		ui_controllable_flag = std::make_shared<CSignalEvent>(FILEMAP_LENS_UI_EVENT);
	}
}

void uninit_license_verified_event()
{
	--event_ref;
	if (event_ref == 0) {
		license_flag = nullptr;
		ui_controllable_flag = nullptr;
	}
}

IPCPrismLens::IPCPrismLens(int cam_index) : camera_index(cam_index)
{
	IPCPrismLens::is_lens_app();
	init_license_verified_event();
}

IPCPrismLens::~IPCPrismLens()
{
	uninit_license_verified_event();
}

void IPCPrismLens::init_ipc_sessions()
{
	ipcVB.init_session(camera_index);
}

void IPCPrismLens::tick()
{
	if (!time_to_tick())
		return;

	if (is_lens_app()) {
		bool prism_run = is_handle_existed(PRISM_RUNNING_FLAG);
		if (!prism_run) {
			std::lock_guard<std::mutex> lock(IPCPrismLens::exit_callback_mutex);
			for (auto &callback : IPCPrismLens::exit_callbacks) {
				callback->reset_prism_variables();
			}
		}
	} else {
		bool lens_run = is_handle_existed(LENS_RUNNING_FLAG);
		if (!lens_run) {
			std::lock_guard<std::mutex> lock(IPCPrismLens::exit_callback_mutex);
			for (auto &callback : IPCPrismLens::exit_callbacks) {
				callback->reset_lens_variables();
			}
		}
	}
}

bool IPCPrismLens::is_license_verified()
{
	return license_flag && license_flag->get_sign();
}

void IPCPrismLens::notify_license_verified(bool verified)
{
	assert(is_lens_app());
	if (license_flag)
		license_flag->set_sign(verified);
}

bool IPCPrismLens::is_lens_ui_controllable()
{
	return ui_controllable_flag && ui_controllable_flag->get_sign();
}

void IPCPrismLens::notify_lens_ui_state(bool ui_controllable)
{
	assert(is_lens_app());
	if (ui_controllable_flag)
		ui_controllable_flag->set_sign(ui_controllable);
}

bool IPCPrismLens::is_vb_removed()
{
	assert(!is_lens_app());
	int index;
	auto header = ipcVB.map_write(index);
	if (!header)
		return false;

	auto vb_removed = (2 == header->vb_type); // 2: VB_WITHOUT_BK

	ipcVB.unmap_write(index);
	return vb_removed;
}

void IPCPrismLens::request_lens_remove_vb(bool need_remove)
{
	int index;
	auto header = ipcVB.map_write(index);
	if (!header)
		return;

	if (need_remove) {
		header->vb_type = 2; // VB_WITHOUT_BK
	} else {
		header->vb_type = header->vb_orignal_type;
	}

	header->prism_request_update = true;
	ipcVB.unmap_write(index);
}

void IPCPrismLens::notify_lens_ui_vb_type(int type)
{
	assert(is_lens_app());
	int index;
	auto header = ipcVB.map_write(index);
	if (!header)
		return;

	// 0: VB_ORIGINAL_WITHOUT_BLUR
	// 1: VB_ORIGINAL_WITH_BLUR
	if (0 == type || 1 == type) {
		header->vb_orignal_type = type;
	}

	// if prism is requesting vb, lens app will not override it
	if (!header->prism_request_update) {
		header->vb_type = type;
	}

	ipcVB.unmap_write(index);
}

int IPCPrismLens::get_prism_requested_vb_type()
{
	assert(is_lens_app());
	int index;
	auto header = ipcVB.map_write(index);
	if (!header)
		return -1;

	int type = -1;
	if (header->prism_request_update) {
		header->prism_request_update = false;
		type = header->vb_type;
	}

	ipcVB.unmap_write(index);
	return type;
}

bool IPCPrismLens::notify_active_state(bool active)
{
	assert(is_lens_app());
	int index;
	auto header = ipcVB.map_write(index);
	if (!header)
		return false;

	header->lens_actived = active;
	ipcVB.unmap_write(index);

	return true;
}

bool IPCPrismLens::notify_capture_state(bool valid)
{
	assert(is_lens_app());
	int index;
	auto header = ipcVB.map_write(index);
	if (!header)
		return false;

	header->capture_valid = valid;
	ipcVB.unmap_write(index);

	return true;
}

bool IPCPrismLens::notify_lens_onoff(bool valid)
{
	assert(is_lens_app());
	int index;
	auto header = ipcVB.map_write(index);
	if (!header)
		return false;

	header->lens_on = valid;
	ipcVB.unmap_write(index);

	return true;
}

bool IPCPrismLens::is_lens_actived()
{
	int index;
	auto header = ipcVB.map_write(index);
	if (!header)
		return false;

	bool ret = header->lens_actived;
	ipcVB.unmap_write(index);

	return ret;
}

bool IPCPrismLens::is_capture_valid()
{
	int index;
	auto header = ipcVB.map_write(index);
	if (!header)
		return false;

	bool ret = header->capture_valid;
	ipcVB.unmap_write(index);

	return ret;
}

bool IPCPrismLens::is_lens_on()
{
	int index;
	auto header = ipcVB.map_write(index);
	if (!header)
		return false;

	bool ret = header->lens_on;
	ipcVB.unmap_write(index);

	return ret;
}

void IPCPrismLens::notify_lens_version(int major, int minor, int build)
{
	assert(is_lens_app());

	int index;
	auto header = ipcVB.map_write(index);
	if (!header)
		return;

	header->lens_ver_major = major;
	header->lens_ver_minor = minor;
	header->lens_ver_build = build;

	bool ret = header->capture_valid;
	ipcVB.unmap_write(index);
}

void IPCPrismLens::get_lens_version(int &major, int &minor, int &build)
{
	major = minor = build = 0;

	int index;
	auto header = ipcVB.map_write(index);
	if (!header)
		return;

	major = header->lens_ver_major;
	minor = header->lens_ver_minor;
	build = header->lens_ver_build;

	bool ret = header->capture_valid;
	ipcVB.unmap_write(index);
}

bool IPCPrismLens::time_to_tick()
{
	static const DWORD64 interval_ms = 500;

	auto tm = GetTickCount64();
	if (tm - pre_tick_time < interval_ms) {
		return false;
	}

	pre_tick_time = tm;
	return true;
}

CSignalEvent::CSignalEvent(const char *name)
{
	assert(name);
	if (name) {
		bool new_create = false;
		event = CHandleWrapper::GetEvent(name, true, &new_create);
		if (!event) {
			assert(false);
			return;
		}

		if (new_create || IPCPrismLens::is_lens_app()) {
			ResetEvent(event);
		}
	}
}

CSignalEvent::~CSignalEvent()
{
	if (!event) {
		return;
	}

	if (IPCPrismLens::is_lens_app()) {
		ResetEvent(event);
	}

	CloseHandle(event);
}

void CSignalEvent::set_sign(bool sign_flag)
{
	assert(IPCPrismLens::is_lens_app());
	if (!event)
		return;

	if (sign_flag) {
		SetEvent(event);
	} else {
		ResetEvent(event);
	}
}

bool CSignalEvent::get_sign()
{
	if (!event)
		return false;

	bool is_signed = (WAIT_OBJECT_0 == WaitForSingleObject(event, 0));
	return is_signed;
}