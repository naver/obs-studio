#pragma once
#include "obs.h"

#ifdef __cplusplus
extern "C" {
#endif

//PRISM/wangshaohui/20260112/PRISM_PC-5037/action log ---------------------------------- start
enum ACTION_HELPER_TYPE {
	ACTION_HELPER_TYPE_SOURCE = 0,
	ACTION_HELPER_TYPE_SCENEITEM,
	ACTION_HELPER_TYPE_SCENE,
};

enum SCENEITEM_ATTRIBUTE_TYPE {
	HIDE_ITEM = 0,
	SHOW_ITEM,
	MOVE_ITEM,
	ROTATE_ITEM,
	SCALE_ITEM,
	CROP_ITEM,
	FIT_STRETCH_TO_SCREEN,
	CENTER_TO_SCREEN,
	FLIP_H_V,
	BELEND_MODE,
	SCALE_FILTER,
	MOVE_ORDER,
	BELEND_METHOD,
	EDIT_TRANSFORM,
	RESET_TRANSFORM,
};

#define PROPERTY_RENDER_TIMEOUT 2000000000 // 2000 ms

#define PROPERTY_NO_CHANGE 0
#define PROPERTY_UI_CHANGED 1
#define PROPERTY_UPDATED 2

#ifdef PLS_UI_ACTION_STATS
extern void *pls_create_action_helper(enum ACTION_HELPER_TYPE type);
extern void pls_free_action_helper(void *ptr, enum ACTION_HELPER_TYPE type);
extern void *pls_get_source_action_helper(obs_source_t *source);
extern void *pls_get_item_action_helper(obs_sceneitem_t *item);
extern void *pls_get_scene_action_helper(obs_scene_t *item);

extern bool pls_is_property_update_delay(); // return delay_update
extern void pls_set_current_source(obs_source_t *source);
extern obs_source_t *pls_get_action_parent(obs_source_t *source);

EXPORT obs_source_t *pls_get_current_source();
EXPORT void pls_on_source_property_changed(obs_source_t *source, const char *property_name);
EXPORT void pls_set_property_update_delay(bool delay_update);
EXPORT void pls_on_source_property_updated(obs_source_t *source);
EXPORT void pls_on_source_property_render(obs_source_t *source, uint64_t timeout_ns);
EXPORT void pls_set_action_parent(obs_source_t *source, obs_source_t *parent);

// step: PROPERTY_UI_CHANGED or PROPERTY_UPDATED
EXPORT void pls_on_item_attribute_changed(obs_sceneitem_t *item, enum SCENEITEM_ATTRIBUTE_TYPE type, int step);
EXPORT void pls_on_item_render(obs_sceneitem_t *item);

EXPORT void pls_on_drawpen_event(obs_scene_t *scene, const char *operation);
EXPORT void pls_on_drawpen_updated(obs_scene_t *scene);
EXPORT void pls_on_drawpen_render(obs_scene_t *scene);

#else
#define pls_get_current_source() NULL
#define pls_on_source_property_changed(...)
#define pls_set_property_update_delay(...)
#define pls_on_source_property_updated(...)
#define pls_on_source_property_render(...)
#define pls_on_item_attribute_changed(...)
#define pls_on_item_render(...)
#define pls_set_current_source(...)
#define pls_set_action_parent(...)

#define pls_on_drawpen_event(...)
#define pls_on_drawpen_updated(...)
#define pls_on_drawpen_render(...)
#endif
//PRISM/wangshaohui/20260112/PRISM_PC-5037/action log ---------------------------------- end

#ifdef __cplusplus
}
#endif
