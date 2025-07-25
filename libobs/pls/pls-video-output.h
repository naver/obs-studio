#pragma once
#include "util/c99defs.h"
//PRISM/wangshaohui/20250409/PRISM_PC-2599/checking video_t

#ifdef __cplusplus
extern "C" {
#endif

// As before, in video_output_close, memory will be freed.
// Now, the video_t struct will not be freed until obs_shutdown.
// We will save all video_t as invalid list from video_output_close.
EXPORT void pls_insert_invalid_video_output(void *video);

EXPORT bool pls_check_output_video(void *output, const char *from_func);

EXPORT void pls_video_output_destrory(const void *video);
EXPORT void pls_destroy_all_video_output();

#ifdef __cplusplus
}
#endif
