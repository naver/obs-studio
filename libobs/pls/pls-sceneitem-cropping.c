#include "pls-sceneitem-cropping.h"
#include "obs-internal.h"
#include "obs-scene.h"

void pls_sceneitem_set_cropping(obs_sceneitem_t *item, bool cropping)
{
	if (!obs_ptr_valid(item, "obs_sceneitem_set_cropping"))
		return;

	if (item->bounds_type != OBS_BOUNDS_NONE)
		return;

	if (item->is_cropping == cropping)
		return;

	item->is_cropping = cropping;
}

bool pls_sceneitem_get_cropping(const obs_sceneitem_t *item)
{
	if (!obs_ptr_valid(item, "obs_sceneitem_get_cropping"))
		return false;

	return item->is_cropping;
}
