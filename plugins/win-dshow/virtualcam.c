#include <obs-module.h>
#include <util/platform.h>
#include "util/threading.h"
#include "shared-memory-queue.h"

struct virtualcam_data {
	obs_output_t *output;
	video_queue_t *vq;
	volatile bool active;
	volatile bool stopping;
	//PRISM/Xiewei/20250207/PRISM_PC-2246/fix: add log
	volatile bool first_video;
};

static const char *virtualcam_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "Virtual Camera Output";
}

static void virtualcam_destroy(void *data)
{
	struct virtualcam_data *vcam = (struct virtualcam_data *)data;
	//PRISM/Xiewei/20250207/PRISM_PC-2246/fix: add log
	blog(LOG_INFO, "%s-%p", __FUNCTION__, (void *)vcam);
	video_queue_close(vcam->vq);
	bfree(data);
}

static void *virtualcam_create(obs_data_t *settings, obs_output_t *output)
{
	struct virtualcam_data *vcam = (struct virtualcam_data *)bzalloc(sizeof(*vcam));
	vcam->output = output;

	UNUSED_PARAMETER(settings);
	//PRISM/Xiewei/20250207/PRISM_PC-2246/fix: add log
	blog(LOG_INFO, "%s-%p", __FUNCTION__, (void *)vcam);
	return vcam;
}

static bool virtualcam_start(void *data)
{
	struct virtualcam_data *vcam = (struct virtualcam_data *)data;
	uint32_t width = obs_output_get_width(vcam->output);
	uint32_t height = obs_output_get_height(vcam->output);

	struct obs_video_info ovi;
	obs_get_video_info(&ovi);

	uint64_t interval = ovi.fps_den * 10000000ULL / ovi.fps_num;

	char res[64];
	snprintf(res, sizeof(res), "%dx%dx%lld", (int)width, (int)height, (long long)interval);

	//PRISM/wangshaohui/20250211/none/PRISM should use different file
	//char *res_file = os_get_config_path_ptr("obs-virtualcam.txt");
	char *res_file = os_get_config_path_ptr("obs-virtualcam-for-prism.txt");
	os_quick_write_utf8_file_safe(res_file, res, strlen(res), false, "tmp", NULL);
	bfree(res_file);

	vcam->vq = video_queue_create(width, height, interval);
	if (!vcam->vq) {
		//PRISM/Xiewei/20250207/PRISM_PC-2246/fix: add log
		//blog(LOG_WARNING, "starting virtual-output failed");
		blog(LOG_WARNING, "%s-%p: starting virtual-output failed", __FUNCTION__, (void *)vcam);
		return false;
	}

	struct video_scale_info vsi = {0};
	vsi.format = VIDEO_FORMAT_NV12;
	vsi.width = width;
	vsi.height = height;
	obs_output_set_video_conversion(vcam->output, &vsi);

	os_atomic_set_bool(&vcam->active, true);
	os_atomic_set_bool(&vcam->stopping, false);
	//PRISM/Xiewei/20250207/PRISM_PC-2246/fix: add log
	os_atomic_set_bool(&vcam->first_video, true);
	blog(LOG_INFO, "Virtual output started");
	obs_output_begin_data_capture(vcam->output, 0);
	//PRISM/Xiewei/20250207/PRISM_PC-2246/fix: add log
	blog(LOG_INFO, "%s-%p", __FUNCTION__, (void *)vcam);
	return true;
}

static void virtualcam_deactive(struct virtualcam_data *vcam)
{
	obs_output_end_data_capture(vcam->output);
	video_queue_close(vcam->vq);
	vcam->vq = NULL;

	os_atomic_set_bool(&vcam->active, false);
	os_atomic_set_bool(&vcam->stopping, false);
	//PRISM/Xiewei/20250207/PRISM_PC-2246/fix: add log
	os_atomic_set_bool(&vcam->first_video, true);

	//PRISM/Xiewei/20250207/PRISM_PC-2246/fix: add log
	//blog(LOG_INFO, "Virtual output stopped");
	blog(LOG_INFO, "%s-%p: Virtual output stopped", __FUNCTION__, (void *)vcam);
}

static void virtualcam_stop(void *data, uint64_t ts)
{
	struct virtualcam_data *vcam = (struct virtualcam_data *)data;
	os_atomic_set_bool(&vcam->stopping, true);
	//PRISM/Xiewei/20250207/PRISM_PC-2246/fix: add log
	os_atomic_set_bool(&vcam->first_video, true);

	//PRISM/Xiewei/20250207/PRISM_PC-2246/fix: add log
	//blog(LOG_INFO, "Virtual output stopping");
	blog(LOG_INFO, "%s-%p: Virtual output stopping", __FUNCTION__, (void *)vcam);

	UNUSED_PARAMETER(ts);
}

static void virtual_video(void *param, struct video_data *frame)
{
	struct virtualcam_data *vcam = (struct virtualcam_data *)param;

	if (!vcam->vq)
		return;

	if (!os_atomic_load_bool(&vcam->active))
		return;

	if (os_atomic_load_bool(&vcam->stopping)) {
		virtualcam_deactive(vcam);
		return;
	}

	//PRISM/Xiewei/20250207/PRISM_PC-2246/fix: add log
	if (os_atomic_load_bool(&vcam->first_video)) {
		blog(LOG_INFO, "%s-%p: first video received in vcam", __FUNCTION__, (void *)vcam);
		os_atomic_set_bool(&vcam->first_video, false);
	}
	video_queue_write(vcam->vq, frame->data, frame->linesize, frame->timestamp);
}

struct obs_output_info virtualcam_info = {
	.id = "virtualcam_output",
	.flags = OBS_OUTPUT_VIDEO,
	.get_name = virtualcam_name,
	.create = virtualcam_create,
	.destroy = virtualcam_destroy,
	.start = virtualcam_start,
	.stop = virtualcam_stop,
	.raw_video = virtual_video,
};
