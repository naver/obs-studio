#pragma once

#include "obs.h"

#ifdef __cplusplus
extern "C" {
#endif
EXPORT void pls_rtmp_connect_success(void *rtmp, void *output);
EXPORT void pls_rtmp_log_event_time_gap(void *rtmp, void *output,
					const char *event);

#ifdef __cplusplus
}
#endif
