#include "pls-video-output.h"
#include "pls-base.h"
#include "obs.h"
#include "util/platform.h"
#include <mutex>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

struct invalid_video_info {
	bool notified = false;
};

std::recursive_mutex lock_videos;
// void*: video_t
std::unordered_map<const void *, std::shared_ptr<invalid_video_info>> invalid_video_list;

void pls_insert_invalid_video_output(void *video)
{
	if (!video)
		return;

	std::lock_guard locker(lock_videos);
	if (invalid_video_list.find(video) == invalid_video_list.end()) {
		invalid_video_list[video] = std::make_shared<invalid_video_info>();
	} else {
		assert(false && "you are freeing video_t repeatedly");
	}
}

void send_signal(const obs_output_t *output, const void *video, const char *from_func)
{
	if (!video || !from_func || !output)
		return;

	const char *id = obs_output_get_id(output);
	if (!id)
		return;

	const char *display_name = obs_output_get_display_name(id);
	if (!display_name)
		return;

	const char *output_name = obs_output_get_name(output);
	if (!output_name)
		return;

	const char *fields[][2] = {
		{"from_func", from_func},
		{"output_id", id},
		{"output_name", output_name},
	};
	blogex(false, LOG_WARNING, fields, 3, "using invalid video from %s, video_t=%p output_id=%s output_name='%s'",
	       from_func, video, id, output_name);

	// this signal is added and registered in OBSBasic::InitOBSCallbacks()
	struct calldata data = {0};
	calldata_set_string(&data, "plugin_name", display_name);
	calldata_set_string(&data, "output_name", output_name);
	signal_handler_signal(obs_get_signal_handler(), "using_invalid_video", &data);
	calldata_free(&data);
}

bool pls_is_video_output_valid(const void *video, const void *output, const char *from_func)
{
	if (!video || !output || !from_func) {
		assert(false);
		return true; // empty pointer, treat it as valid to continue following logic
	}

	bool found = false;
	bool need_signal = false;

	{
		std::lock_guard locker(lock_videos);
		auto itr = invalid_video_list.find(video);
		if (itr != invalid_video_list.end() && itr->second != NULL) {
			found = true;

			uint64_t current_time = os_gettime_ns();
			if (!itr->second->notified) {         // never notified
				itr->second->notified = true; // notify once
				need_signal = true;
			}
		}
	}

	if (!found)
		return true; // it is valid

	if (need_signal)
		send_signal((const obs_output_t *)output, video, from_func);

	assert(false && "using invalid video_t");
	return false; // video has been closed
}

bool pls_check_output_video(void *o, const char *from_func)
{
	if (!o || !from_func) {
		assert(false);
		return true; // empty pointer, treat it as valid to continue following logic
	}

	obs_output_t *output = (obs_output_t *)o;
	uint32_t flags = obs_output_get_flags(output);

	bool include_video = (flags & OBS_OUTPUT_VIDEO) != 0;
	if (!include_video)
		return true; // no video data, return ok

	bool use_encoded_data = (flags & OBS_OUTPUT_ENCODED) != 0;
	if (use_encoded_data) {
		for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
			obs_encoder_t *ve = obs_output_get_video_encoder2(output, i);
			if (!ve)
				continue;

			video_t *v = obs_encoder_video(ve);
			if (!v)
				continue;

			if (!pls_is_video_output_valid(v, output, from_func))
				return false; // found invalid video
		}

		return true;

	} else {
		video_t *v = obs_output_video(output);
		if (!v)
			return true; // treat empty pointer as valid

		return pls_is_video_output_valid(v, output, from_func);
	}
}

void pls_destroy_all_video_output()
{
	std::lock_guard locker(lock_videos);
	for (const auto &pair : invalid_video_list) {
		pls_video_output_destrory(pair.first);
	}
	invalid_video_list.clear();
}
