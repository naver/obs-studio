#pragma once

#include "../util/base.h"

#ifdef __cplusplus
extern "C" {
#endif

//PRISM/Zhangdewen/20210218/#/extend log support nelo fields
typedef void (*log_handler_ex_t)(bool kr, int lvl, const char *msg, va_list args, const char *fields[][2],
				 int field_count, void *param);
EXPORT void base_get_log_handler_ex(log_handler_ex_t *handler, void **param);
EXPORT void base_set_log_handler_ex(log_handler_ex_t handler, void *param);
EXPORT void blogvaex(int log_level, const char *format, va_list args, const char *fields[][2], int field_count);
EXPORT void blogex(bool kr, int log_level, const char *fields[][2], int field_count, const char *format, ...);

//PRISM/Zengqin/20211201/#/Notify the log process, the child process exited abnormally
EXPORT void bexception(const char *process_name, const char *pid, const char *src);

#if defined(PLS_UI_ACTION_STATS)
#define PLS_UI_ACTION_OBS(format, ...) blogex(false, LOG_UI_ACTION, NULL, 0, format, ##__VA_ARGS__)
#else
#define PLS_UI_ACTION_OBS(format, ...)
#endif

//PRISM/Zhongling/20230816/#2251/crash on `gl_update`
#ifdef __APPLE__
EXPORT void os_async_on_main_queue(void *context, void (*callback)(void *context));
#endif

//PRISM/Zhongling/20231122/#/add func stacks
EXPORT void pls_get_func_stacks(char *info, size_t info_size, int max_layer);

//PRISM/jimboRen/20250325/#/add global field, for obs send field to logger exe
typedef void (*log_global_field_handler_t)(const char *key, const char *value);
EXPORT void base_set_log_global_field_handler_cn(log_global_field_handler_t handler);
EXPORT void base_add_global_field_cn(const char *key, const char *value);

//PRISM/chenguoxi/20251021/PRISM_PC-4242/sre for encoder
//PRISM/lizhiyong/20251022/PRISM_PC-4243/sre for output
typedef struct {
	uint64_t total_encoder_frames;
	uint64_t total_encoder_dropped_frames;
	uint64_t total_output_frames;
	uint64_t total_output_dropped_frames;
	uint64_t total_stream_frames;
	uint64_t total_stream_dropped_frames;
	uint64_t total_render_frames;
	uint64_t total_render_dropped_frames;
} SRE_FRAME_INFO;

//PRISM/lizhiyong/20251126/PRISM_PC-4622/add render drop types
typedef struct {
	uint64_t sleep_start_count;
	uint64_t sleep_count;
	uint64_t lanuch_count;
	uint64_t last_cpu_gpu_high_count;
	uint64_t cpu_gpu_high_count;
	uint64_t self_cpu_gpu_high_count;
} FRAME_DROP_ESTIMATE_INFO;

enum FRAME_DROP_REASON {
	UNKNOW,
	OS_SLEEP,
	GPU_CPU_HIGH,
	GPU_CPU_HIGH_BY_SELF,
	LAUNCH_LONG,
	NO_DROP,
};

EXPORT extern SRE_FRAME_INFO g_sre_frame_info;
EXPORT extern FRAME_DROP_ESTIMATE_INFO g_frame_drop_estimate_info;
EXPORT double pls_smart_rate(uint64_t base, uint64_t total);
EXPORT void pls_update_stream_frames(uint64_t total_frames, uint64_t dropped_frames);
EXPORT void pls_update_render_frames(uint64_t total_frames, uint64_t dropped_frames);
EXPORT void pls_upload_frame_type(void);
EXPORT void pls_set_launch_render_drop_count(uint64_t dropped_frames);
EXPORT void pls_set_sys_gpu_cpu_value(double total_cpu, double total_gpu, double process_cpu, double process_gpu,
				      uint64_t dropped_frames);
EXPORT void pls_sleep_start(uint64_t dropped_frames);
EXPORT void pls_sleep_end(uint64_t dropped_frames);

#define PLS_DROP_FRAME_NLOG_REASON_LEN 32

struct pls_drop_frame_nlog_fields {
	int video_encoder_total_frames;
	int video_encoder_drop_frames;
	double video_encoder_drop_percent;
	int output_total_frames;
	int output_drop_frames;
	double output_drop_percent;
	int stream_total_frames;
	int stream_drop_frames;
	double stream_drop_percent;
	int render_total_frames;
	int render_drop_frames;
	double render_drop_percent;
	char render_drop_reason[PLS_DROP_FRAME_NLOG_REASON_LEN];
};

EXPORT bool pls_get_drop_frame_nlog_fields(struct pls_drop_frame_nlog_fields *fields);

//PRISM/EricLi/20260618/PRISM_PC-6044/callback for drop-frame nlog triggered from pls_upload_frame_type
typedef void (*pls_drop_frame_nlog_fn_t)(const struct pls_drop_frame_nlog_fields *fields);
EXPORT void pls_register_drop_frame_nlog_fn(pls_drop_frame_nlog_fn_t fn);

//---------------------------------------------- taken time module --------------------------------------------------
// Note: During app's startup or switching scene collector, {enable} should be false to ignore the taken time records
typedef void (*pls_graphic_callback_t)();
EXPORT void pls_set_graphic_callback(pls_graphic_callback_t cb);
extern void pls_notify_graphic_event();

EXPORT void pls_enable_render_drop(bool enable);
extern bool pls_is_render_drop_enabled();

static const uint64_t time_ns_3ms = 3000000;
extern void pls_set_render_thread();
EXPORT void pls_begin_taken_time(void *obj, const char *obj_plugin, const char *desc);
EXPORT void pls_end_taken_time(void *obj, const char *obj_plugin, const char *desc, uint64_t min_ns);

EXPORT void pls_stop_upload_time();

#ifdef _WIN32
// This function cannot be used in the release version. It's just for debug issue.
EXPORT void pls_dump_texture(void *texture, wchar_t const *path);
#endif

#ifdef __cplusplus
}
#endif
