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

#ifdef __cplusplus
}
#endif
