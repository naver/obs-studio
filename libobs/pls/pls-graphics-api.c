#include "pls-graphics-api.h"
#include "graphics/graphics.c"

gs_stagesurf_t *gs_device_canvas_map(uint32_t *cx, uint32_t *cy, enum gs_color_format *fmt, uint8_t **data, uint32_t *linesize)
{
	graphics_t *graphics = thread_graphics;

	if (!gs_valid("gs_device_canvas_map"))
		return NULL;

	//if (graphics->exports.device_canvas_map)
	//return graphics->exports.device_canvas_map(graphics->device, cx, cy, fmt, data, linesize);

	return NULL;
}

void gs_device_canvas_unmap(gs_stagesurf_t *surface)
{
	graphics_t *graphics = thread_graphics;

	if (!gs_valid("gs_device_canvas_unmap"))
		return;

	//if (graphics->exports.device_canvas_unmap)
		//graphics->exports.device_canvas_unmap(graphics->device, surface);
}

gs_adapters_info_t *gs_adapter_get_info()
{
	graphics_t *graphics = thread_graphics;

	if (!gs_valid("gs_adapter_get_info"))
		return NULL;

	//if (graphics->exports.adapter_get_info)
		//return graphics->exports.adapter_get_info(graphics->device);

	return NULL;
}
