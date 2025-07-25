#pragma once

#include "obs.h"

#ifdef __cplusplus
extern "C" {
#endif
EXPORT void pls_rtmp_connect_success(void *rtmp, void *output);
EXPORT void pls_rtmp_log_event_time_gap(void *rtmp, void *output, const char *event);

EXPORT void pls_output_reset_send_time(obs_output_t *output);
EXPORT void pls_output_on_video_sent(obs_output_t *output, uint64_t time_ns);
EXPORT double pls_output_avg_send_time_ms(obs_output_t *output); // in ms

#ifdef __cplusplus
}
#endif
