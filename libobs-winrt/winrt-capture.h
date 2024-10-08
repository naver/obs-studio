#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <obs-module.h>

#ifdef __cplusplus
extern "C" {
#endif

EXPORT BOOL winrt_capture_supported();
EXPORT BOOL winrt_capture_cursor_toggle_supported();
EXPORT struct winrt_capture *winrt_capture_init_window(BOOL cursor, HWND window,
						       BOOL client_area,
						       BOOL force_sdr);
EXPORT struct winrt_capture *
winrt_capture_init_monitor(BOOL cursor, HMONITOR monitor, BOOL force_sdr);

//PRISM/WangShaohui/20240201/#4181/Use wgc for region capture
EXPORT struct winrt_capture *winrt_capture_init_desktop(BOOL cursor,
							HMONITOR monitor,
							BOOL force_sdr,
							RECT tex_region);

EXPORT void winrt_capture_free(struct winrt_capture *capture);

EXPORT BOOL winrt_capture_active(const struct winrt_capture *capture);
EXPORT BOOL winrt_capture_show_cursor(struct winrt_capture *capture,
				      BOOL visible);
EXPORT enum gs_color_space
winrt_capture_get_color_space(const struct winrt_capture *capture);
EXPORT void winrt_capture_render(struct winrt_capture *capture);
EXPORT uint32_t winrt_capture_width(const struct winrt_capture *capture);
EXPORT uint32_t winrt_capture_height(const struct winrt_capture *capture);

EXPORT void winrt_capture_thread_start();
EXPORT void winrt_capture_thread_stop();

//PRISM/Xiewei/20240416/#5080/reset border required property when display changed
EXPORT void winrt_capture_on_display_changed();

#ifdef __cplusplus
}
#endif
