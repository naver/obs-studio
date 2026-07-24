#include "pls-base.h"
#include "util/platform.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef __APPLE__
#include <execinfo.h>
#endif

static int LOG_SUBPROCESS_EXCEPTION = -100;

//PRISM/chenguoxi/20251021/PRISM_PC-4242/sre for encoder
SRE_FRAME_INFO g_sre_frame_info = {0};

FRAME_DROP_ESTIMATE_INFO g_frame_drop_estimate_info = {0};

//PRISM/Zhangdewen/20210218/#/extend log support nelo fields
static void def_log_handler_ex(bool kr, int log_level, const char *format, va_list args, const char *fields[][2],
			       int field_count, void *param)
{
	char out[4096];
	vsnprintf(out, sizeof(out), format, args);

	switch (log_level) {
	case LOG_DEBUG:
		fprintf(stdout, "debug: %s\n", out);
		fflush(stdout);
		break;
	case LOG_INFO:
		fprintf(stdout, "info: %s\n", out);
		fflush(stdout);
		break;
	case LOG_WARNING:
		fprintf(stdout, "warning: %s\n", out);
		fflush(stdout);
		break;
	case LOG_ERROR:
		fprintf(stderr, "error: %s\n", out);
		fflush(stderr);
	}

	UNUSED_PARAMETER(kr);
	UNUSED_PARAMETER(fields);
	UNUSED_PARAMETER(field_count);
	UNUSED_PARAMETER(param);
}

//PRISM/Zhangdewen/20210218/#/extend log support nelo fields
static log_handler_ex_t log_handler_ex = def_log_handler_ex;
static void *log_param_ex = NULL;

//PRISM/Zhangdewen/20210218/#/extend log support nelo fields
void base_get_log_handler_ex(log_handler_ex_t *handler, void **param)
{
	if (handler)
		*handler = log_handler_ex;
	if (param)
		*param = log_param_ex;
}

//PRISM/Zhangdewen/20210218/#/extend log support nelo fields
void base_set_log_handler_ex(log_handler_ex_t handler, void *param)
{
	if (!handler)
		handler = def_log_handler_ex;

	log_param_ex = param;
	log_handler_ex = handler;
}

//PRISM/Zhangdewen/20210218/#/extend log support nelo fields
void blogvaex(int log_level, const char *format, va_list args, const char *fields[][2], int field_count)
{
	log_handler_ex(false, log_level, format, args, fields, field_count, log_param_ex);
}

//PRISM/Zhangdewen/20210218/#/extend log support nelo fields
void blogex(bool kr, int log_level, const char *fields[][2], int field_count, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	log_handler_ex(kr, log_level, format, args, fields, field_count, log_param_ex);
	va_end(args);
}

//PRISM/Zengqin/20211201/#/Notify the log process, the child process exited abnormally
void bexception(const char *process_name, const char *pid, const char *src)
{
	const char *fields[][2] = {{"process", process_name}, {"pid", pid}, {"src", src}};
	blogex(false, LOG_SUBPROCESS_EXCEPTION, fields, 3, NULL);
}

void pls_get_func_stacks(char *info, size_t info_size, int max_layer)
{
	if (!info || info_size < 10)
		return;

#ifdef __APPLE__
	memset(info, 0, info_size);
	void *callstack[128] = {NULL};
	int frames = backtrace(callstack, sizeof(callstack) / sizeof(callstack[0]));
	char **strs = backtrace_symbols(callstack, frames);
	unsigned long len = 0;

	for (int i = 0; i < frames && i < max_layer; ++i) {
		if (strs[i] && len + 2 < info_size) {
			int src_len = strlen(strs[i]);
			int dst_len = info_size - len - 2;
			int copy_len = src_len < dst_len ? src_len : dst_len;
			strncpy(info + len, strs[i], copy_len);
			strncpy(info + len + copy_len, "\n", 1);
		}
		len = strlen(info);
	}
	free(strs);
#endif
}

//PRISM/jimboRen/20250325/#/add global field, for obs send field to logger exe
static log_global_field_handler_t log_global_field_handler = NULL;
void base_set_log_global_field_handler_cn(log_global_field_handler_t handler)
{
	log_global_field_handler = handler;
}
void base_add_global_field_cn(const char *key, const char *value)
{
	if (!log_global_field_handler) {
		return;
	}
	log_global_field_handler(key, value);
}

//PRISM/chenguoxi/20251021/PRISM_PC-4242/sre for encoder
double pls_smart_rate(uint64_t base, uint64_t total)
{
	if (total == 0) {
		return 0.0;
	}

	double result = ((double)base * 100.0) / (double)total;

	if (result < 0.0) {
		result = 0.0;
	} else if (result > 100.0) {
		result = 100.0;
	}

	return result;
}
//PRISM/lizhiyong/20251022/PRISM_PC-4243/sre for stream output and render
void pls_update_stream_frames(uint64_t total_frames, uint64_t dropped_frames)
{
	g_sre_frame_info.total_stream_frames += total_frames;
	g_sre_frame_info.total_stream_dropped_frames += dropped_frames;
}

void pls_update_render_frames(uint64_t total_frames, uint64_t dropped_frames)
{
	g_sre_frame_info.total_render_frames = total_frames;
	g_sre_frame_info.total_render_dropped_frames = dropped_frames;
}

void pls_set_launch_render_drop_count(uint64_t dropped_frames)
{
	g_frame_drop_estimate_info.lanuch_count = dropped_frames;
}

void pls_sleep_start(uint64_t dropped_frames)
{
	g_frame_drop_estimate_info.sleep_start_count = dropped_frames;
}

void pls_sleep_end(uint64_t dropped_frames)
{
	if (g_frame_drop_estimate_info.sleep_start_count > 0) {
		g_frame_drop_estimate_info.sleep_count += dropped_frames - g_frame_drop_estimate_info.sleep_start_count;
	}
	g_frame_drop_estimate_info.sleep_start_count = 0;
}

void pls_set_sys_gpu_cpu_value(double total_cpu, double total_gpu, double process_cpu, double process_gpu,
			       uint64_t dropped_frames)
{
	if (total_cpu <= 0.0f || total_gpu <= 0.0f) {
		return;
	}

	const double THRESHOLD = 80.0;

	if (total_cpu >= THRESHOLD || total_gpu >= THRESHOLD) {
		uint64_t drop_count_since_last = dropped_frames - g_frame_drop_estimate_info.last_cpu_gpu_high_count;
		g_frame_drop_estimate_info.cpu_gpu_high_count += drop_count_since_last;
		if (process_cpu / total_cpu > 0.6f || process_gpu / total_gpu > 0.6f) {
			g_frame_drop_estimate_info.self_cpu_gpu_high_count += drop_count_since_last;
		}
	}
	g_frame_drop_estimate_info.last_cpu_gpu_high_count = dropped_frames;
}

enum FRAME_DROP_REASON estimate_render_drop_reason()
{
	double render_rate =
		pls_smart_rate(g_sre_frame_info.total_render_dropped_frames, g_sre_frame_info.total_render_frames);

	if (render_rate == 0.0)
		return NO_DROP;

	double render_rate_threshold = render_rate * 0.5;

	double sleep_rate =
		pls_smart_rate(g_frame_drop_estimate_info.sleep_count, g_sre_frame_info.total_render_frames);

	if (sleep_rate > render_rate_threshold)
		return OS_SLEEP;

	double cpu_gpu_high_rate =
		pls_smart_rate(g_frame_drop_estimate_info.cpu_gpu_high_count, g_sre_frame_info.total_render_frames);

	if (cpu_gpu_high_rate > render_rate_threshold) {
		double prism_cpu_gpu_high_percent = pls_smart_rate(g_frame_drop_estimate_info.self_cpu_gpu_high_count,
								   g_frame_drop_estimate_info.cpu_gpu_high_count);
		if (prism_cpu_gpu_high_percent > 50.0)
			return GPU_CPU_HIGH_BY_SELF;
		else
			return GPU_CPU_HIGH;
	}

	double launch_long_rate =
		pls_smart_rate(g_frame_drop_estimate_info.lanuch_count, g_sre_frame_info.total_render_frames);

	if (launch_long_rate > render_rate_threshold)
		return LAUNCH_LONG;

	return UNKNOW;
}

#define SRE_FRAME_INFO_FIELD_COUNT 13

//PRISM/EricLi/20260624/PRISM_PC-none/snprintf %.1f is locale-sensitive; replace comma with dot for NELO double fields
static void snprintf_locale_safe(char *buf, size_t size, double val)
{
	snprintf(buf, size, "%.1f", val);
	for (char *p = buf; *p; p++)
		if (*p == ',')
			*p = '.';
}

void convert_frame_info_to_strings(char out_strs[SRE_FRAME_INFO_FIELD_COUNT][32], size_t str_size, char *log)
{
	snprintf(out_strs[0], str_size, "%llu", (unsigned long long)g_sre_frame_info.total_encoder_frames);
	snprintf(out_strs[1], str_size, "%llu", (unsigned long long)g_sre_frame_info.total_encoder_dropped_frames);
	snprintf_locale_safe(out_strs[2], str_size,
			     pls_smart_rate(g_sre_frame_info.total_encoder_dropped_frames, g_sre_frame_info.total_encoder_frames));
	snprintf(out_strs[3], str_size, "%llu", (unsigned long long)g_sre_frame_info.total_output_frames);
	snprintf(out_strs[4], str_size, "%llu", (unsigned long long)g_sre_frame_info.total_output_dropped_frames);
	snprintf_locale_safe(out_strs[5], str_size,
			     pls_smart_rate(g_sre_frame_info.total_output_dropped_frames, g_sre_frame_info.total_output_frames));
	snprintf(out_strs[6], str_size, "%llu", (unsigned long long)g_sre_frame_info.total_stream_frames);
	snprintf(out_strs[7], str_size, "%llu", (unsigned long long)g_sre_frame_info.total_stream_dropped_frames);
	snprintf_locale_safe(out_strs[8], str_size,
			     pls_smart_rate(g_sre_frame_info.total_stream_dropped_frames, g_sre_frame_info.total_stream_frames));
	snprintf(out_strs[9], str_size, "%llu", (unsigned long long)g_sre_frame_info.total_render_frames);
	snprintf(out_strs[10], str_size, "%llu", (unsigned long long)g_sre_frame_info.total_render_dropped_frames);
	snprintf_locale_safe(out_strs[11], str_size,
			     pls_smart_rate(g_sre_frame_info.total_render_dropped_frames, g_sre_frame_info.total_render_frames));

	enum FRAME_DROP_REASON reason = estimate_render_drop_reason();
	switch (reason) {
	case OS_SLEEP:
		snprintf(out_strs[12], str_size, "%s", "os_sleep");
		break;
	case GPU_CPU_HIGH:
		snprintf(out_strs[12], str_size, "%s", "gpu_cpu_high");
		break;
	case GPU_CPU_HIGH_BY_SELF:
		snprintf(out_strs[12], str_size, "%s", "gpu_cpu_high_by_self");
		break;
	case LAUNCH_LONG:
		snprintf(out_strs[12], str_size, "%s", "launch_long");
		break;
	case NO_DROP:
		snprintf(out_strs[12], str_size, "%s", "no_drop");
		break;
	default:
		snprintf(out_strs[12], str_size, "%s", "unknown");
		break;
	}

	snprintf(log, 200, "encoder(%s,%s,%s),output(%s,%s,%s),stream(%s,%s,%s),render(%s,%s,%s,%s)", out_strs[0],
		 out_strs[1], out_strs[2], out_strs[3], out_strs[4], out_strs[5], out_strs[6], out_strs[7], out_strs[8],
		 out_strs[9], out_strs[10], out_strs[11], out_strs[12]);
}

bool pls_get_drop_frame_nlog_fields(struct pls_drop_frame_nlog_fields *fields)
{
	if (!fields)
		return false;
	char strs[SRE_FRAME_INFO_FIELD_COUNT][32];
	char log[200];
	convert_frame_info_to_strings(strs, sizeof(strs[0]), log);
	fields->video_encoder_total_frames = (int)strtoull(strs[0], NULL, 10);
	fields->video_encoder_drop_frames = (int)strtoull(strs[1], NULL, 10);
	fields->video_encoder_drop_percent = strtod(strs[2], NULL);
	fields->output_total_frames = (int)strtoull(strs[3], NULL, 10);
	fields->output_drop_frames = (int)strtoull(strs[4], NULL, 10);
	fields->output_drop_percent = strtod(strs[5], NULL);
	fields->stream_total_frames = (int)strtoull(strs[6], NULL, 10);
	fields->stream_drop_frames = (int)strtoull(strs[7], NULL, 10);
	fields->stream_drop_percent = strtod(strs[8], NULL);
	fields->render_total_frames = (int)strtoull(strs[9], NULL, 10);
	fields->render_drop_frames = (int)strtoull(strs[10], NULL, 10);
	fields->render_drop_percent = strtod(strs[11], NULL);
	snprintf(fields->render_drop_reason, sizeof(fields->render_drop_reason), "%s", strs[12]);
	return true;
}

static pls_drop_frame_nlog_fn_t g_drop_frame_nlog_fn = NULL;

void pls_register_drop_frame_nlog_fn(pls_drop_frame_nlog_fn_t fn)
{
	g_drop_frame_nlog_fn = fn;
}

void pls_upload_frame_type(void)
{
#ifdef NDEBUG
	char strs[SRE_FRAME_INFO_FIELD_COUNT][32];
	char log[200];
	convert_frame_info_to_strings(strs, sizeof(strs[0]), log);
	const char *fields[][2] = {
		{"video_encoder_total_frames", strs[0]}, {"video_encoder_drop_frames", strs[1]},
		{"video_encoder_drop_percent", strs[2]}, {"output_total_frames", strs[3]},
		{"output_drop_frames", strs[4]},         {"output_drop_percent", strs[5]},
		{"stream_total_frames", strs[6]},        {"stream_drop_frames", strs[7]},
		{"stream_drop_percent", strs[8]},        {"render_total_frames", strs[9]},
		{"render_drop_frames", strs[10]},        {"render_drop_percent", strs[11]},
		{"render_drop_reason", strs[12]},
	};

	blogex(false, LOG_INFO, fields, sizeof(fields) / sizeof(fields[0]), log);

	if (g_drop_frame_nlog_fn) {
		struct pls_drop_frame_nlog_fields nlog_fields;
		if (pls_get_drop_frame_nlog_fields(&nlog_fields))
			g_drop_frame_nlog_fn(&nlog_fields);
	}
#endif

}
