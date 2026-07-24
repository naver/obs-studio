#pragma once
#include "pls-c-util.h"
#include "pls-cs.hpp"

extern "C" {
#include "graphics-hook.h"
}

CCriticalSection lock_flags;
bool notify_flags[RenderType_Count] = {false};

const char *render_type_key = "pls_game_render_type";
void send_game_render_type(enum RenderType type)
{
	bool need_send = false;

	{
		CAutoLockSection lock(lock_flags);
		if (notify_flags[type] == false) {
			notify_flags[type] = true;
			need_send = true;
		}
	}

	if (need_send) {
		switch (type) {
		case RenderType_D3D8:
			hlog("%s=%s", render_type_key, "d3d8");
			break;
		case RenderType_D3D9:
			hlog("%s=%s", render_type_key, "d3d9");
			break;
		case RenderType_D3D10:
			hlog("%s=%s", render_type_key, "d3d10");
			break;
		case RenderType_D3D11:
			hlog("%s=%s", render_type_key, "d3d11");
			break;
		case RenderType_D3D12:
			hlog("%s=%s", render_type_key, "d3d12");
			break;
		case RenderType_OpenGL:
			hlog("%s=%s", render_type_key, "opengl");
			break;
		case RenderType_Vulkan:
			hlog("%s=%s", render_type_key, "vulkan");
			break;
		default:
			assert(false);
			break;
		}
	}
}
