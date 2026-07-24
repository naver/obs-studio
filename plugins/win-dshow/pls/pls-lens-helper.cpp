#include "pls-lens-helper.h"
#include "util/base.h"
#include "util/platform.h"
#include <obs-module.h>
#include <obs.hpp>
#include <util/dstr.hpp>
#include <util/platform.h>
#include <util/windows/WinHandle.hpp>
#include <util/threading.h>
#include "encode-dstr.hpp"
#include "pls/pls-source.h"
#include <map>

#define VIDEO_DEVICE_ID "video_device_id"
#define AUDIO_DEVICE_ID "audio_device_id"

#define V2_KEY_LENS_VIDEO_INDEX "lens_video_index" // value should be 0/1/2
#define V2_KEY_LENS_AUDIO_INDEX "lens_audio_index" // value should be 0/1/2
#define V2_KEY_LENS_ACTIVE "active"
#define V2_USE_CUSTOM_AUDIO "use_custom_audio_device"
#define V2_FLIP_IMAGE "flip_vertically"
#define V2_FLIP_IMAGE_H "flip_horizontally"

#define DSHOW_USE_CUSTOM_AUDIO "use_custom_audio_device"
#define DSHOW_FLIP_IMAGE "flip_vertically"
#define DSHOW_FLIP_IMAGE_H "flip_horizontally"

#define DSHOW_KEY_LENS_ACTIVE "active"

#define NAME_PATH_DELIMITER ":"

// this map is used to convert old device id to new index (video)
// leftValue: value of "video_device_id", rightValue: index of lens
const std::map<std::string, int> map_video_id = {
	{std::string(TEXT_PRISM_LENS_1) + NAME_PATH_DELIMITER, 0},
	{std::string(TEXT_PRISM_LENS_2) + NAME_PATH_DELIMITER, 1},
	{std::string(TEXT_PRISM_LENS_3) + NAME_PATH_DELIMITER, 2},
};

// this map is used to convert old device id to new index (audio)
// leftValue: value of "audio_device_id", rightValue: index of lens
const std::map<std::string, int> map_audio_id = {
	{std::string(TEXT_PRISM_LENS_AUDIO_1) + NAME_PATH_DELIMITER, 0},
	{std::string(TEXT_PRISM_LENS_AUDIO_2) + NAME_PATH_DELIMITER, 1},
	{std::string(TEXT_PRISM_LENS_AUDIO_3) + NAME_PATH_DELIMITER, 2},
};

struct lens_device_data {
	size_t idx = 0;
	bool enabled = true;
	std::string audio_name;
	std::string video_name;
	int cx = 0;
	int cy = 0;
};

struct lens_device_id {
	std::wstring audio_w;
	std::string audio;
	std::wstring video_w;
	std::string video;
};

// Note: you should not change the string value
static const lens_device_id lens_device_ids[MAX_LENS_COUNT] = {
	{TEXT_LENS_AUDIO_PATHW_1, TEXT_LENS_AUDIO_PATH_1, TEXT_LENS_VIDEO_PATHW_1, TEXT_LENS_VIDEO_PATH_1},
	{TEXT_LENS_AUDIO_PATHW_2, TEXT_LENS_AUDIO_PATH_2, TEXT_LENS_VIDEO_PATHW_2, TEXT_LENS_VIDEO_PATH_2},
	{TEXT_LENS_AUDIO_PATHW_3, TEXT_LENS_AUDIO_PATH_3, TEXT_LENS_VIDEO_PATHW_3, TEXT_LENS_VIDEO_PATH_3},
};

void pls_enum_lens_device_callback(void *param, size_t idx, const char *audio_name, const char *video_name,
				   bool is_enabled, int cx, int cy)
{
	if (!audio_name || !video_name || !param || idx >= MAX_LENS_COUNT) {
		assert(false);
		return;
	}
	if (!is_enabled) {
		// if device is not enabled, we do not add it to the list
		return;
	}

	std::vector<lens_device_data> *devices = (std::vector<lens_device_data> *)param;
	if (devices->size() >= MAX_LENS_COUNT) {
		assert(false);
		return;
	}

	lens_device_data data;
	data.enabled = is_enabled;
	data.idx = idx;
	data.audio_name = audio_name;
	data.video_name = video_name;
	data.cx = cx;
	data.cy = cy;

	devices->push_back(data);
}

std::wstring utf8_to_wstring(const std::string &str)
{
	wchar_t *res = NULL;
	os_utf8_to_wcs_ptr(str.c_str(), 0, &res);

	if (!res) {
		assert(false);
		return L"";
	}

	std::wstring wstr(res);
	bfree(res);

	return wstr;
}

void insert_lens_video_devices(vector<VideoDevice> &devices)
{
	static bool lens_v2_loaded = LoadLibraryW(L"prism-lens-v2.dll");
	if (!lens_v2_loaded)
		return;

	std::vector<lens_device_data> data;
	pls_enum_lens_devices(pls_enum_lens_device_callback, &data);

	for (int i = (int)data.size() - 1; i >= 0; --i) {
		const auto &item = data[i];

		auto name = utf8_to_wstring(item.video_name);
		if (name.empty()) {
			assert(false);
			continue;
		}

		VideoInfo cap;
		cap.minCX = cap.maxCX = item.cx;
		cap.minCY = cap.maxCY = item.cy;
		cap.granularityCX = cap.granularityCY = 1;
		cap.minInterval = cap.maxInterval = 333333;
		cap.format = VideoFormat::ARGB;

		VideoDevice lensV;
		lensV.name = name;
		lensV.path = lens_device_ids[item.idx].video_w;
		lensV.caps.push_back(cap);

		devices.insert(devices.begin(), lensV);
	}
}

void insert_lens_audio_devices(vector<AudioDevice> &devices)
{
	static bool lens_v2_loaded = LoadLibraryW(L"prism-lens-v2.dll");
	if (!lens_v2_loaded)
		return;

	std::vector<lens_device_data> data;
	pls_enum_lens_devices(pls_enum_lens_device_callback, &data);

	for (int i = (int)data.size() - 1; i >= 0; --i) {
		const auto &item = data[i];

		auto name = utf8_to_wstring(item.audio_name);
		if (name.empty()) {
			assert(false);
			continue;
		}

		AudioDevice lensA;
		lensA.name = name;
		lensA.path = lens_device_ids[item.idx].audio_w;

		devices.insert(devices.begin(), lensA);
	}
}

void on_selected_device_changed(string video_device_id, obs_properties_t *props)
{
	bool is_lens_device = video_device_id.find(LENS_PATH_PREFIX) != std::string::npos;
	bool enable_ui = !is_lens_device;

	// These properties are not supported by new lens source
	static std::vector<std::string> keys = {"video_config", "xbar_config",  "color_space",    "color_range",
						"buffering",    "autorotation", "hw_decode",      "audio_output_mode",
						"res_type",     "resolution",   "frame_interval", "video_format"};

	for (const auto &item : keys) {
		auto p = obs_properties_get(props, item.c_str());
		assert(p);
		if (p) {
			obs_property_set_visible(p, enable_ui);
		}
	}
}

bool find_lens_device_index(const std::vector<lens_device_data> &data, int i, lens_device_data &output)
{
	for (const auto &item : data) {
		if (item.idx == i) {
			output = item;
			return true;
		}
	}

	return false;
}

void check_device_value(obs_data_t *settings, const std::string &key, const std::vector<lens_device_data> &data,
			bool is_video)
{
	/* old lens eg:
	        "video_device_id": "PRISM Lens 2:"
                "audio_device_id": "PRISM Lens audio 3:"
	*/

	const char *id_str = obs_data_get_string(settings, key.c_str());
	if (!id_str) {
		assert(false);
		return;
	}

	const std::map<std::string, int> &map_dev = is_video ? map_video_id : map_audio_id;
	auto itr = map_dev.find(id_str);
	if (itr == map_dev.end())
		return;

	auto index = itr->second;
	lens_device_data info;
	if (find_lens_device_index(data, index, info)) {
		const auto device_id = is_video ? lens_device_ids[index].video : lens_device_ids[index].audio;
		const auto device_name = is_video ? info.video_name : info.audio_name;

		std::string value = device_name;
		value += std::string(NAME_PATH_DELIMITER);
		value += device_id;

		obs_data_set_string(settings, key.c_str(), value.c_str());
	}
}

void adapt_lens_settings(obs_data_t *settings)
{
	std::vector<lens_device_data> data;
	pls_enum_lens_devices(pls_enum_lens_device_callback, &data);

	// convert settings of old lens to new lens
	check_device_value(settings, VIDEO_DEVICE_ID, data, true);
	check_device_value(settings, AUDIO_DEVICE_ID, data, false);
}

void reset_lens_source(obs_source_t *lens_v2)
{
	if (lens_v2) {
		obs_data_t *settings = obs_source_get_settings(lens_v2);
		obs_data_set_bool(settings, V2_KEY_LENS_ACTIVE, false);
		obs_data_set_bool(settings, V2_USE_CUSTOM_AUDIO, false);
		obs_data_set_bool(settings, V2_FLIP_IMAGE, false);
		obs_data_set_bool(settings, V2_FLIP_IMAGE_H, false);
		obs_source_update(lens_v2, settings);
		obs_data_release(settings);
	}
}

int get_lens_device_idx(const std::wstring &path)
{
	for (int i = 0; i < MAX_LENS_COUNT; i++) {
		const auto &item = lens_device_ids[i];
		if (item.video_w == path || item.audio_w == path) {
			return i;
		}
	}

	return -1;
}

static inline void reroute_lens_audio(obs_source_t *target, obs_source_t *src)
{
	if (!src)
		return;

	proc_handler_t *ph = obs_source_get_proc_handler(src);
	calldata_t cd = {0};
	calldata_set_ptr(&cd, "target", target);
	proc_handler_call(ph, "reroute_audio", &cd);
	calldata_free(&cd);
}

void force_new_timestamp(obs_source_t *lens_v2)
{
	if (!lens_v2)
		return;

	proc_handler_t *ph = obs_source_get_proc_handler(lens_v2);
	proc_handler_call(ph, "force_new_timestamp", nullptr);
}

void check_use_lens(obs_source_t *dshow_source, obs_source_t *&lens_v2, obs_data_t *settings, bool &use_lens_video,
		    int &video_idx, bool &use_lens_audio)
{
	use_lens_video = use_lens_audio = false;

	string video_device_id = obs_data_get_string(settings, VIDEO_DEVICE_ID);
	string audio_device_id = obs_data_get_string(settings, AUDIO_DEVICE_ID);
	bool use_custom_audio = obs_data_get_bool(settings, DSHOW_USE_CUSTOM_AUDIO);
	bool flip_image = obs_data_get_bool(settings, DSHOW_FLIP_IMAGE);
	bool flip_image_h = obs_data_get_bool(settings, DSHOW_FLIP_IMAGE_H);
	bool actived = obs_data_get_bool(settings, DSHOW_KEY_LENS_ACTIVE);

	video_idx = -1;
	int v_idx = -1;
	int a_idx = -1;

	DeviceId vid;
	if (DecodeDeviceId(vid, video_device_id.c_str())) {
		v_idx = get_lens_device_idx(vid.path);
	}

	if (use_custom_audio) {
		DeviceId aid;
		if (DecodeDeviceId(aid, audio_device_id.c_str())) {
			a_idx = get_lens_device_idx(aid.path);
		}
	}

	if (v_idx >= 0 || a_idx >= 0) {
		if (!lens_v2) {
			lens_v2 = obs_source_create_private(TEXT_PRISM_LENS_ID, "dshow_lens_v2_private", NULL);
			if (!lens_v2) {
				pls_set_lens_camera(dshow_source, false);
				assert(false);
				return;
			}

			blog(LOG_INFO, "[DShow Device: '%s' %p] lens source is created for camera, ptr = %p",
			     obs_source_get_name(dshow_source), dshow_source, lens_v2);

			obs_source_inc_showing(lens_v2);
		}

		obs_data_t *settings = obs_source_get_settings(lens_v2);
		obs_data_set_int(settings, V2_KEY_LENS_VIDEO_INDEX, v_idx);
		obs_data_set_int(settings, V2_KEY_LENS_AUDIO_INDEX, a_idx);
		obs_data_set_bool(settings, V2_USE_CUSTOM_AUDIO, use_custom_audio);
		obs_data_set_bool(settings, V2_KEY_LENS_ACTIVE, actived);
		obs_data_set_bool(settings, V2_FLIP_IMAGE, flip_image);
		obs_data_set_bool(settings, V2_FLIP_IMAGE_H, flip_image_h);
		obs_source_update(lens_v2, settings);
		obs_data_release(settings);

		use_lens_video = v_idx >= 0;
		use_lens_audio = a_idx >= 0;
	}

	if (use_lens_video) {
		obs_source_output_video2(dshow_source, nullptr);
		pls_set_lens_camera(dshow_source, true);
		video_idx = v_idx;
	} else {
		pls_set_lens_camera(dshow_source, false);
	}

	if (use_lens_audio) {
		obs_source_set_audio_active(dshow_source, lens_v2 != nullptr);
		if (lens_v2)
			reroute_lens_audio(dshow_source, lens_v2);
	} else {
		if (lens_v2)
			reroute_lens_audio(nullptr, lens_v2);
	}

	blog(LOG_INFO, "[DShow Device: '%s' %p] settings updated: video_lens_index=%d audio_lens_index=%d",
	     obs_source_get_name(dshow_source), dshow_source, v_idx, a_idx);
}

bool use_lens_video(obs_data_t *settings)
{
	string video_device_id = obs_data_get_string(settings, VIDEO_DEVICE_ID);
	int v_idx = -1;

	DeviceId vid;
	if (DecodeDeviceId(vid, video_device_id.c_str())) {
		v_idx = get_lens_device_idx(vid.path);
	}
	bool use_lens = v_idx >= 0;
	return use_lens;
}
