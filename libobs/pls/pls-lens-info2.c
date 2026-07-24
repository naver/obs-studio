#include "pls-lens-info.h"
#include "obs-internal.h"
#ifdef _WIN32

EXPORT void pls_set_lens_camera(obs_source_t *source, bool is_lens_camera)
{
	if (source) {
		source->is_lens_camera = is_lens_camera;
	}
}

EXPORT bool pls_is_lens_camera(obs_source_t *source)
{
	return source && source->is_lens_camera;
}

//--------------------------------------------------------------------------------------------------------------
#endif // _WIN32
