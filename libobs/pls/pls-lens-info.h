#pragma once
#include "obs.h"

// defined in lens app
#define MAX_LENS_COUNT 3

// plugin id
#define TEXT_PRISM_LENS_ID "prism_lens"
#define TEXT_PRISM_LENS_MOBILE_ID "prism_lens_mobile"

#ifdef _WIN32 // ----------------------------------- Windows start -----------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

#define LENS_PATH_PREFIX "lens_device_path_prefix_123e4567-e89b-12d3-a456-426614174000"
#define LENS_PATH_PREFIX_W L"lens_device_path_prefix_123e4567-e89b-12d3-a456-426614174000"

#define TO_STR(s) #s
#define PLS_MAKE_LENS_PATH(type, index) LENS_PATH_PREFIX "-" TO_STR(type) "-" TO_STR(index)

#define TO_STR_W(s) L"" #s
#define PLS_MAKE_LENS_PATH_W(type, index) LENS_PATH_PREFIX_W L"-" TO_STR_W(type) L"-" TO_STR_W(index)

static const char *TEXT_LENS_VIDEO_PATH_1 = PLS_MAKE_LENS_PATH(video, 1);
static const char *TEXT_LENS_VIDEO_PATH_2 = PLS_MAKE_LENS_PATH(video, 2);
static const char *TEXT_LENS_VIDEO_PATH_3 = PLS_MAKE_LENS_PATH(video, 3);
static const char *TEXT_LENS_AUDIO_PATH_1 = PLS_MAKE_LENS_PATH(audio, 1);
static const char *TEXT_LENS_AUDIO_PATH_2 = PLS_MAKE_LENS_PATH(audio, 2);
static const char *TEXT_LENS_AUDIO_PATH_3 = PLS_MAKE_LENS_PATH(audio, 3);

static const wchar_t *TEXT_LENS_VIDEO_PATHW_1 = PLS_MAKE_LENS_PATH_W(video, 1);
static const wchar_t *TEXT_LENS_VIDEO_PATHW_2 = PLS_MAKE_LENS_PATH_W(video, 2);
static const wchar_t *TEXT_LENS_VIDEO_PATHW_3 = PLS_MAKE_LENS_PATH_W(video, 3);
static const wchar_t *TEXT_LENS_AUDIO_PATHW_1 = PLS_MAKE_LENS_PATH_W(audio, 1);
static const wchar_t *TEXT_LENS_AUDIO_PATHW_2 = PLS_MAKE_LENS_PATH_W(audio, 2);
static const wchar_t *TEXT_LENS_AUDIO_PATHW_3 = PLS_MAKE_LENS_PATH_W(audio, 3);

EXPORT void pls_init_lens_resolution();
EXPORT void pls_check_lens_resolution(uint32_t lens_index);
EXPORT void pls_set_lens_resolution(uint32_t lens_index, int width, int height);
EXPORT void pls_get_lens_resolution(uint32_t lens_index, int *width, int *height);

typedef void (*pls_enum_lens_device_callback_t)(void *param, size_t idx, const char *audio_name, const char *video_name,
						bool is_enabled, int cx, int cy);
EXPORT void pls_enum_lens_devices(pls_enum_lens_device_callback_t callback, void *param);

EXPORT void pls_set_lens_camera(obs_source_t *source, bool is_lens_camera);
EXPORT bool pls_is_lens_camera(obs_source_t *source);

#ifdef __cplusplus
}
#endif
// ----------------------------------- Windows end -----------------------------------------
#elif __APPLE__
static const char *UUID_PRISM_LEN1 = "3A5E8421-E5F2-4672-85CF-42B75B2B6E1A";
static const char *UUID_PRISM_LEN2 = "098937AC-2646-4FB4-AE43-AE1B3169E5C9";
static const char *UUID_PRISM_LEN3 = "8AFA123E-E08E-4311-B10A-0165988BD502";

#ifdef __cplusplus
extern "C" {
#endif

EXPORT void pls_init_lens_resolution();
EXPORT void pls_get_lens_resolution(uint32_t lens_index, int *width, int *height);

#ifdef __cplusplus
}
#endif
#endif
