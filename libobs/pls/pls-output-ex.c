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
