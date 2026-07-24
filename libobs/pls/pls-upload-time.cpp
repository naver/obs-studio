#include "pls-taken-time-impl.h"

#ifdef _MSC_VER
#define THREAD_LOCAL __declspec(thread)
#else
#define THREAD_LOCAL __thread
#endif

THREAD_LOCAL bool is_render_thread = false;
time_uploader time_instance;

std::recursive_mutex lock_graphics_cb;
pls_graphic_callback_t graphics_cb = nullptr;

//---------------------------------------------------------------------------------------------------------------
void pls_set_graphic_callback(pls_graphic_callback_t cb)
{
	std::lock_guard<std::recursive_mutex> lock(lock_graphics_cb);
	graphics_cb = cb;
}

void pls_notify_graphic_event()
{
	std::lock_guard<std::recursive_mutex> lock(lock_graphics_cb);
	if (graphics_cb)
		graphics_cb();
}

void pls_enable_render_drop(bool enable)
{
	time_uploader::render_drop_enabled = enable;
}

bool pls_is_render_drop_enabled()
{
	return time_uploader::render_drop_enabled;
}

void pls_set_render_thread()
{
	is_render_thread = true;
}

void pls_begin_taken_time(void *obj, const char *obj_plugin, const char *desc)
{
	if (is_render_thread)
		time_instance.begin_taken_time(obj, obj_plugin, desc);
}

void pls_end_taken_time(void *obj, const char *obj_plugin, const char *desc, uint64_t min_ns)
{
	if (is_render_thread)
		time_instance.end_taken_time(obj, obj_plugin, desc, min_ns);
}

void pls_stop_upload_time()
{
	time_instance.stop();
}
