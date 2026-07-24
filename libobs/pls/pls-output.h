#pragma once

#include "obs.h"

#ifdef __cplusplus
extern "C" {
#endif

struct pls_platform_info {
	bool filled;
	size_t hash_streamkey;
	char url[256];
	char channel_name[128];
	char channel_type[128];
};

EXPORT void pls_rtmp_connect_success(void *rtmp, void *output);
EXPORT void pls_rtmp_log_event_time_gap(void *rtmp, void *output, const char *event);

EXPORT void pls_output_reset_send_time(obs_output_t *output);
EXPORT void pls_output_on_video_sent(obs_output_t *output, uint64_t time_ns);
EXPORT double pls_output_avg_send_time_ms(obs_output_t *output); // in ms

EXPORT void pls_increase_order(obs_output_t *output, int *old_value);
EXPORT void pls_set_platform_info(obs_output_t *output, const struct pls_platform_info *info);
EXPORT void pls_log_output_stop(obs_output_t *output, int code);

EXPORT bool pls_is_output_encoder_error(obs_output_t *output);

#ifdef __cplusplus
}
#endif
