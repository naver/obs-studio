#pragma once
#include <Windows.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

enum RenderType {
	RenderType_D3D8 = 0, // must begin from 0
	RenderType_D3D9,
	RenderType_D3D10,
	RenderType_D3D11,
	RenderType_D3D12,
	RenderType_OpenGL,
	RenderType_Vulkan,
	RenderType_Count,
};

//PRISM/wangshaohui/20240801/PRISM_PC-846/add sre for render type
void send_game_render_type(enum RenderType type);

#ifdef __cplusplus
}
#endif
