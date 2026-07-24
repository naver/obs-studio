#include "prism-ipc-impl.h"

#include <notify.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <string>

namespace {
constexpr uint64_t VB_TYPE_MASK = 0x7ULL;          // bits 0-2
constexpr uint64_t VB_ORIGINAL_MASK = 0x38ULL;     // bits 3-5
constexpr uint64_t VB_ORIGINAL_SHIFT = 3;
constexpr uint64_t VB_REQUEST_FLAG = 1ULL << 8;    // prism_request_update flag
// VB state layout packs current/original types and a prism-request flag.

std::string to_lower(std::string v)
{
	std::transform(v.begin(), v.end(), v.begin(),
		       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return v;
}
} // namespace

bool IPCPrismLens::is_lens_app()
{
	static bool initialized = false;
	static bool is_lens = false;

	if (initialized)
		return is_lens;

	const char *name = getprogname();
	std::string exe = name ? name : "";
	auto lower = to_lower(exe);
	is_lens = lower.find("prismlens") != std::string::npos;

	initialized = true;
	return is_lens;
}

IPCPrismLens::IPCPrismLens(int cam_index)
	: camera_index(cam_index)
	, lens_app(is_lens_app())
{
}

IPCPrismLens::~IPCPrismLens()
{
	if (license_token && notify_is_valid_token(license_token)) {
		notify_cancel(license_token);
	}
	if (vb_token && notify_is_valid_token(vb_token)) {
		notify_cancel(vb_token);
	}
	if (active_token && notify_is_valid_token(active_token)) {
		notify_cancel(active_token);
	}
	if (capture_token && notify_is_valid_token(capture_token)) {
		notify_cancel(capture_token);
	}
	if (lens_onoff_token && notify_is_valid_token(lens_onoff_token)) {
		notify_cancel(lens_onoff_token);
	}
	if (ui_controllable_token && notify_is_valid_token(ui_controllable_token)) {
		notify_cancel(ui_controllable_token);
	}
}

void IPCPrismLens::init_ipc_sessions()
{
	// Lazily created when first used; ensure tokens exist to prime state.
	ensure_token(license_token, license_channel());
	ensure_token(vb_token, vb_channel());
	ensure_token(active_token, active_channel());
	ensure_token(capture_token, capture_channel());
	ensure_token(lens_onoff_token, lens_onoff_channel());
	ensure_token(ui_controllable_token, ui_controllable_channel());
}

void IPCPrismLens::tick()
{
	// Keep parity with Windows signature; nothing to poll on macOS.
}

bool IPCPrismLens::is_license_verified()
{
	uint64_t state = 0;
	if (!read_state(license_token, license_channel(), state))
		return false;
	return state != 0;
}

void IPCPrismLens::notify_license_verified(bool verified)
{
	write_state(license_token, license_channel(), verified ? 1 : 0);
}

bool IPCPrismLens::is_vb_removed()
{
	uint64_t state = 0;
	if (!read_state(vb_token, vb_channel(), state))
		return false;

	int vb_type = 0, vb_original = 0;
	bool prism_request = false;
	decode_vb_state(state, vb_type, vb_original, prism_request);
	return vb_type == VB_WITHOUT_BK;
}

void IPCPrismLens::request_lens_remove_vb(bool need_remove)
{
	uint64_t state = 0;
	int vb_type = VB_ORIGINAL_WITHOUT_BLUR;
	int vb_original = cached_vb_original;
	bool prism_request = false;

	if (read_state(vb_token, vb_channel(), state)) {
		decode_vb_state(state, vb_type, vb_original, prism_request);
	}

	int next_type = need_remove ? VB_WITHOUT_BK : vb_original;
	auto next_state = encode_vb_state(next_type, vb_original, true /*prism_request*/);
	write_state(vb_token, vb_channel(), next_state);
}

void IPCPrismLens::notify_lens_ui_vb_type(int vb_type)
{
	uint64_t state = 0;
	int cur_vb_type = VB_ORIGINAL_WITHOUT_BLUR;
	int vb_original = cached_vb_original;
	bool prism_request = false;

	if (read_state(vb_token, vb_channel(), state)) {
		decode_vb_state(state, cur_vb_type, vb_original, prism_request);
	}

	if (vb_type == VB_ORIGINAL_WITHOUT_BLUR || vb_type == VB_ORIGINAL_WITH_BLUR) {
		vb_original = vb_type;
		cached_vb_original = vb_type;
	}

	// Only override vb_type if prism is not asking to force a value.
	if (!prism_request) {
		cur_vb_type = vb_type;
	}

	auto next_state = encode_vb_state(cur_vb_type, vb_original, prism_request);
	write_state(vb_token, vb_channel(), next_state);
}

int IPCPrismLens::get_prism_requested_vb_type()
{
	assert(lens_app && "get_prism_requested_vb_type should be called in lens app");

	uint64_t state = 0;
	if (!read_state(vb_token, vb_channel(), state))
		return -1;

	int vb_type = VB_ORIGINAL_WITHOUT_BLUR;
	int vb_original = cached_vb_original;
	bool prism_request = false;

	decode_vb_state(state, vb_type, vb_original, prism_request);

	if (!prism_request)
		return -1;

	auto cleared = encode_vb_state(vb_type, vb_original, false);
	write_state(vb_token, vb_channel(), cleared);
	return vb_type;
}

bool IPCPrismLens::notify_active_state(bool active)
{
	return write_state(active_token, active_channel(), active ? 1 : 0);
}

bool IPCPrismLens::is_capture_valid()
{
	uint64_t state = 0;
	if (!read_state(capture_token, capture_channel(), state))
		return false;
	return state != 0;
}

bool IPCPrismLens::notify_capture_state(bool valid)
{
	return write_state(capture_token, capture_channel(), valid ? 1 : 0);
}

bool IPCPrismLens::notify_lens_onoff(bool is_on)
{
	return write_state(lens_onoff_token, lens_onoff_channel(), is_on ? 1 : 0);
}

bool IPCPrismLens::is_lens_on()
{
	uint64_t state = 0;
	if (!read_state(lens_onoff_token, lens_onoff_channel(), state))
		return false;
	return state != 0;
}

bool IPCPrismLens::is_lens_ui_controllable()
{
	uint64_t state = 0;
	if (!read_state(ui_controllable_token, ui_controllable_channel(), state))
		return false;
	return state != 0;
}

void IPCPrismLens::notify_lens_ui_state(bool ui_controllable)
{
	write_state(ui_controllable_token, ui_controllable_channel(), ui_controllable ? 1 : 0);
}

bool IPCPrismLens::ensure_token(int &token, const std::string &name)
{
	if (token && notify_is_valid_token(token))
		return true;

	if (token && !notify_is_valid_token(token)) {
		notify_cancel(token);
		token = 0;
	}

	uint32_t status = notify_register_check(name.c_str(), &token);
	if (status != NOTIFY_STATUS_OK) {
		token = 0;
		return false;
	}
	return true;
}

bool IPCPrismLens::read_state(int &token, const std::string &name, uint64_t &out_state)
{
	if (!ensure_token(token, name)) {
		return false;
	}

	uint64_t state = 0;
	uint32_t status = notify_get_state(token, &state);
	if (status != NOTIFY_STATUS_OK)
		return false;

	out_state = state;
	return true;
}

bool IPCPrismLens::write_state(int &token, const std::string &name, uint64_t state)
{
	if (!ensure_token(token, name))
		return false;

	if (notify_set_state(token, state) != NOTIFY_STATUS_OK)
		return false;

	notify_post(name.c_str());
	return true;
}

std::string IPCPrismLens::license_channel() const
{
	return "com.prism.ipc." + std::to_string(camera_index) + ".license";
}

std::string IPCPrismLens::vb_channel() const
{
	return "com.prism.ipc." + std::to_string(camera_index) + ".vb";
}

std::string IPCPrismLens::active_channel() const
{
	return "com.prism.ipc." + std::to_string(camera_index) + ".active";
}

std::string IPCPrismLens::capture_channel() const
{
	return "com.prism.ipc." + std::to_string(camera_index) + ".capture";
}

std::string IPCPrismLens::lens_onoff_channel() const
{
	return "com.prism.ipc." + std::to_string(camera_index) + ".lens_onoff";
}

std::string IPCPrismLens::ui_controllable_channel() const
{
	return "com.prism.ipc." + std::to_string(camera_index) + ".ui_controllable";
}

uint64_t IPCPrismLens::encode_vb_state(int vb_type, int vb_original_type, bool prism_request) const
{
	uint64_t state = 0;
	state |= (static_cast<uint64_t>(vb_type) & VB_TYPE_MASK);
	state |= (static_cast<uint64_t>(vb_original_type) & VB_TYPE_MASK) << VB_ORIGINAL_SHIFT;
	if (prism_request)
		state |= VB_REQUEST_FLAG;
	return state;
}

void IPCPrismLens::decode_vb_state(uint64_t state, int &vb_type, int &vb_original_type,
				   bool &prism_request)
{
	vb_type = static_cast<int>(state & VB_TYPE_MASK);
	vb_original_type = static_cast<int>((state & VB_ORIGINAL_MASK) >> VB_ORIGINAL_SHIFT);
	prism_request = (state & VB_REQUEST_FLAG) != 0;

	// Keep a cached original for fallback when state is not yet initialized.
	if (vb_original_type >= VB_ORIGINAL_WITHOUT_BLUR && vb_original_type <= VB_WITH_BK) {
		// vb_original_type only intended for original range (0/1) but keep within enum range.
		cached_vb_original = vb_original_type;
	}
}


