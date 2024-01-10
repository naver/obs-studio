#include "pls-base.h"

#include <stdlib.h>
#include <stdio.h>

static int LOG_SUBPROCESS_EXCEPTION = -100;

//PRISM/Zhangdewen/20210218/#/extend log support nelo fields
static void def_log_handler_ex(bool kr, int log_level, const char *format,
			       va_list args, const char *fields[][2],
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
void blogvaex(int log_level, const char *format, va_list args,
	      const char *fields[][2], int field_count)
{
	log_handler_ex(false, log_level, format, args, fields, field_count,
		       log_param_ex);
}

//PRISM/Zhangdewen/20210218/#/extend log support nelo fields
void blogex(bool kr, int log_level, const char *fields[][2], int field_count,
	    const char *format, ...)
{
	va_list args;
	va_start(args, format);
	log_handler_ex(kr, log_level, format, args, fields, field_count,
		       log_param_ex);
	va_end(args);
}

//PRISM/Zengqin/20211201/#/Notify the log process, the child process exited abnormally
void bexception(const char *process_name, const char *pid, const char *src)
{
	const char *fields[][2] = {
		{"process", process_name}, {"pid", pid}, {"src", src}};
	blogex(false, LOG_SUBPROCESS_EXCEPTION, fields, 3, NULL);
}
