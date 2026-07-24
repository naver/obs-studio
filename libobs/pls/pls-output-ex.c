#include "pls-output.h"
#include "pls-base.h"
#include "obs-internal.h"

#define CALC_SPEED_INTERVAL 3000000000ULL // 3s

void pls_output_reset_send_time(obs_output_t *output)
{
	output->avg_send_speed = 0.0;
	output->start_time = 0;
	output->total_video_count = 0;
	output->token_sent_time = 0;
}

void pls_output_on_video_sent(obs_output_t *output, uint64_t time_ns)
{
	uint64_t now_time = os_gettime_ns();

	if (output->start_time == 0) {
		output->start_time = now_time;
		output->total_video_count = 0;
		output->token_sent_time = 0;
	}

	output->total_video_count += 1;
	output->token_sent_time += time_ns;

	if (now_time - output->start_time >= CALC_SPEED_INTERVAL && output->total_video_count) {
		output->avg_send_speed = (double)output->token_sent_time / (double)(output->total_video_count);

		output->start_time = 0;
		output->total_video_count = 0;
		output->token_sent_time = 0;
	}
}

double pls_output_avg_send_time_ms(obs_output_t *output)
{
	return output->avg_send_speed / 1000000.0;
}

void pls_increase_order(obs_output_t *output, int *old_value)
{
	if (!output) {
		assert(false);
		return;
	}

	if (old_value) {
		*old_value = output->start_order;
	}

	++output->start_order;
}

bool pls_is_output_encoder_error(obs_output_t *output)
{
	if (output == NULL) {
		return false;
	}

	for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
		obs_encoder_t *vencoder = output->video_encoders[i];
		if (vencoder && vencoder->last_error_message) {
			return true;
		}
	}

	for (size_t i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++) {
		obs_encoder_t *aencoder = output->audio_encoders[i];
		if (aencoder && aencoder->last_error_message) {
			return true;
		}
	}

	return false;
}

void pls_set_platform_info(obs_output_t *output, const struct pls_platform_info *info)
{
	if (!output || !info) {
		assert(false);
		return;
	}

	memcpy(&output->platform_info, info, sizeof(struct pls_platform_info));
}

void pls_log_output_stop(obs_output_t *output, int code)
{
	char str_output[50] = {0};
	snprintf(str_output, sizeof(str_output), "%p", output);

	char str_code[8] = {0};
	snprintf(str_code, sizeof(str_code), "%d", code);

	const char *name = obs_output_get_name(output);
	const char *output_name = name ? name : "empty_output_name";

	int level = (OBS_OUTPUT_SUCCESS == code) ? LOG_INFO : LOG_WARNING;

	if (output->platform_info.filled) { // this is streaming output
		char str_hash[40] = {0};
		snprintf(str_hash, sizeof(str_hash), "%zu", output->platform_info.hash_streamkey);

		char str_order[8] = {0};
		snprintf(str_order, sizeof(str_order), "%d", output->start_order);

		const char *fields[][2] = {
			{PTS_LOG_TYPE, PTS_TYPE_EVENT},
			{"output", str_output},
			{"output_id", output->info.id},
			{"output_name", output_name},
			{"stop_code", str_code},
			{"start_order", str_order},
			{"hash_streamkey", str_hash},
			{"channel_name", output->platform_info.channel_name},
			{"channel_type", output->platform_info.channel_type},
			{"url", output->platform_info.url},
		};

		blogex(false, level, fields, 10,
		       "%p-obs_output_signal_stop: output_signal [Enter] id:%s data=%p, video=%p, encoder=%p code=%d [stream abort] \n"
		       " \t start_order=%s \n"
		       " \t hash_streamkey=%s \n"
		       " \t channel_name=%s \n"
		       " \t channel_type=%s \n"
		       " \t url=%s \n",
		       output, output->info.id, output->context.data, output->video, output->video_encoders[0], code,
		       str_order, str_hash, output->platform_info.channel_name, output->platform_info.channel_type,
		       output->platform_info.url);

	} else {
		const char *fields[][2] = {
			{PTS_LOG_TYPE, PTS_TYPE_EVENT}, {"output", str_output},  {"output_id", output->info.id},
			{"output_name", output_name},   {"stop_code", str_code},
		};

		blogex(false, level, fields, 5,
		       "%p-obs_output_signal_stop: output_signal [Enter] id:%s data=%p, video=%p, encoder=%p code=%d",
		       output, output->info.id, output->context.data, output->video, output->video_encoders[0], code);
	}
}
