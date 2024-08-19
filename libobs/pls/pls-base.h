#pragma once

#include "../util/base.h"

#ifdef __cplusplus
extern "C" {
#endif

//PRISM/Zhangdewen/20210218/#/extend log support nelo fields
typedef void (*log_handler_ex_t)(bool kr, int lvl, const char *msg,
				 va_list args, const char *fields[][2],
				 int field_count, void *param);
EXPORT void base_get_log_handler_ex(log_handler_ex_t *handler, void **param);
EXPORT void base_set_log_handler_ex(log_handler_ex_t handler, void *param);
EXPORT void blogvaex(int log_level, const char *format, va_list args,
		     const char *fields[][2], int field_count);
EXPORT void blogex(bool kr, int log_level, const char *fields[][2],
		   int field_count, const char *format, ...);

//PRISM/Zengqin/20211201/#/Notify the log process, the child process exited abnormally
EXPORT void bexception(const char *process_name, const char *pid,
		       const char *src);

//PRISM/Zhongling/20230816/#2251/crash on `gl_update`
#ifdef __APPLE__
EXPORT void os_async_on_main_queue(void *context, void (*callback)(void *context));
#endif

//PRISM/Zhongling/20231122/#/add func stacks
EXPORT void pls_get_func_stacks(char *info, size_t info_size, int max_layer);

#ifdef __cplusplus
}
#endif
