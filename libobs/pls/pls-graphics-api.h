#pragma once

#include "util/c99defs.h"
#include "obs.h"
#include "graphics/graphics.h"
#include "graphics/graphics-internal.h"

#ifdef __cplusplus
extern "C" {
#endif
struct gs_adapters_info {
	int current_index;
	DARRAY(struct adapter_info) adapters;
};

typedef struct gs_adapters_info gs_adapters_info_t;

struct texture_map_info {
	uint8_t *data[MAX_AV_PLANES];
	uint32_t linesize[MAX_AV_PLANES];
};

/*graphics apis*/
EXPORT gs_stagesurf_t *gs_device_canvas_map(uint32_t *cx, uint32_t *cy, enum gs_color_format *fmt, uint8_t **data, uint32_t *linesize);
EXPORT void gs_device_canvas_unmap(gs_stagesurf_t *surface);
EXPORT gs_adapters_info_t *gs_adapter_get_info();

#ifdef __cplusplus
}
#endif
