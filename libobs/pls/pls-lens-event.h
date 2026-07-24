#pragma once
#include "obs.h"
#include "pls/pls-lens-info.h"
#include <functional>

struct LensEvents {
	std::function<void(bool running)> lens_running_cb = nullptr;
	std::function<void()> license_verified_cb = nullptr;

	std::function<void(int lens, bool actived)> active_cb = nullptr;
	std::function<void(int lens, bool valid)> capture_cb = nullptr;
	std::function<void(int lens, bool is_on)> onoff_cb = nullptr;
	std::function<void(int lens, bool vb_removed)> vb_state_vb = nullptr;
};

EXPORT void pls_start_monitor_lens_events();
EXPORT void pls_stop_monitor_lens_events();

EXPORT void pls_register_lens_events(void *receiver, const LensEvents &cbs);
EXPORT void pls_unregister_lens_events(void *receiver);

EXPORT bool pls_is_lens_active(int cam_index);
EXPORT bool pls_is_lens_capture_ready(int cam_index);
EXPORT bool pls_is_lens_on(int cam_index);

EXPORT bool pls_is_lens_running();
EXPORT bool pls_is_lens_license_verified();
EXPORT bool pls_is_lens_ui_controllable();
EXPORT bool pls_is_lens_vb_removed(int cam_index);
EXPORT void pls_request_lens_remove_vb(int cam_index, bool need_remove);
