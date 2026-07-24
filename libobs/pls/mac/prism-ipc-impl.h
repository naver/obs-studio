// macOS notifyd-based IPC between PRISM and Lens. Mirrors the Windows IPC
// surface but uses notifyd channels for simple state sharing.
#pragma once

#include <cstdint>
#include <string>

#ifndef VB_ORIGINAL_WITHOUT_BLUR
#define VB_ORIGINAL_WITHOUT_BLUR 0
#define VB_ORIGINAL_WITH_BLUR 1
#define VB_WITHOUT_BK 2
#define VB_WITH_BK 3
#endif

class IPCPrismLens {
public:
	static bool is_lens_app();

	explicit IPCPrismLens(int cam_index);
	~IPCPrismLens();

	void init_ipc_sessions();
	void tick();

	//---------------------------- license -----------------------------
	bool is_license_verified();
	void notify_license_verified(bool verified);

	//---------------------------- VB -----------------------------
	bool is_vb_removed();
	void request_lens_remove_vb(bool need_remove);

	void notify_lens_ui_vb_type(int vb_type);
	int get_prism_requested_vb_type();

	//---------------------------- active state -----------------------------
	bool notify_active_state(bool active);
	bool is_capture_valid();
	bool notify_capture_state(bool valid);
	//---------------------------- lens on/off state -----------------------------
	bool notify_lens_onoff(bool is_on);
	bool is_lens_on();
	//---------------------------- UI controllable -----------------------------
	bool is_lens_ui_controllable();
	void notify_lens_ui_state(bool ui_controllable);

private:
	bool ensure_token(int &token, const std::string &name);
	bool read_state(int &token, const std::string &name, uint64_t &out_state);
	bool write_state(int &token, const std::string &name, uint64_t state);

	std::string license_channel() const;
	std::string vb_channel() const;
	std::string active_channel() const;
	std::string capture_channel() const;
	std::string lens_onoff_channel() const;
	std::string ui_controllable_channel() const;

	uint64_t encode_vb_state(int vb_type, int vb_original_type, bool prism_request) const;
	void decode_vb_state(uint64_t state, int &vb_type, int &vb_original_type,
			     bool &prism_request);

private:
	const int camera_index = -1;
	bool lens_app = false;

	int license_token = 0;
	int vb_token = 0;
	int active_token = 0;
	int capture_token = 0;
	int lens_onoff_token = 0;

	int ui_controllable_token = 0;

	int cached_vb_original = VB_ORIGINAL_WITHOUT_BLUR;
};


