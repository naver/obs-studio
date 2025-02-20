#pragma once

#include "obs.h"

#define ID_IS_VERTICAL "is_vertical"

#ifdef __cplusplus
extern "C" {
#endif

//PRISM/chenguoxi/20241104/PRISM_PC-1452/dual output
/** Add view means turning on dual output. */
EXPORT video_t *pls_vertical_view_add(struct obs_video_info *ovi);

/** Remove view means turning off dual output */
EXPORT void pls_vertical_view_remove();

EXPORT void pls_render_vertical_main_texture_src_color_only();

EXPORT void pls_render_vertical_main_texture();

EXPORT void pls_source_video_render_vertical(obs_source_t *source);

EXPORT void pls_source_video_render_landscape(obs_source_t *source);

/** Only UI turn on, but cannot ensure obs init done. */
EXPORT void pls_set_dual_output_on(bool is_on);

/** Only UI turn on, but cannot ensure obs init done. */
EXPORT bool pls_is_dual_output_on();

/** The dual output init done */
EXPORT bool pls_is_dual_output_initialized();

EXPORT bool pls_get_vertical_video_info(struct obs_video_info *ovi);

EXPORT struct obs_core_video_mix *pls_get_vertical_mix();

EXPORT video_t *pls_get_vertical_video_t(); 

EXPORT obs_sceneitem_t *pls_vertical_scene_add(obs_scene_t *scene,
					       obs_source_t *source,
					       obs_sceneitem_t *insert_after,
					       obs_data_t *settings);

EXPORT bool pls_is_vertical_sceneitem(obs_sceneitem_t *item);

EXPORT bool pls_is_vertical_scene_or_group(obs_source_t *source);

/** Enumerates sources include landscape and vertical */
EXPORT obs_data_array_t *pls_save_sources_filtered_all(obs_save_source_filter_cb cb,
						void *data_);

EXPORT obs_data_t *pls_save_source_smart(obs_source_t *source, bool bypass_vertical);

/** Enumerates sources within a scene include landscape and vertical */
EXPORT void pls_scene_enum_items_all(obs_scene_t *scene,
				     bool (*callback)(obs_scene_t *,
						      obs_sceneitem_t *,
						      void *),
				     void *param);

EXPORT obs_data_t *pls_scene_save_transform_states_all(obs_scene_t *scene,
						       bool all_items);


EXPORT void pls_sceneitem_group_enum_items_all(
	obs_sceneitem_t *group,
	bool (*callback)(obs_scene_t *, obs_sceneitem_t *, void *),
	void *param);

EXPORT void pls_enum_hotkeys_all(obs_hotkey_enum_func func, void *data);


EXPORT void pls_enum_hotkey_bindings_all(obs_hotkey_binding_enum_func func,
				     void *data);

EXPORT bool pls_is_vertical_hotkey(obs_hotkey_t *hotkey);

/** pls_scene_save_all: save all include vertical */
/** scene_save: only save landscape */
EXPORT void pls_scene_save_all(obs_scene_t *scene, obs_data_t *settings);

/** Don't use them for rendering. For tick and so on. */
EXPORT uint32_t pls_source_get_vertical_width(obs_source_t *source);
EXPORT uint32_t pls_source_get_vertical_height(obs_source_t *source);

/** Only for rendering*/
EXPORT bool pls_get_video_info_current(struct obs_video_info *ovi);

/** Only for rendering*/
EXPORT bool pls_sceneitem_is_rendering(obs_sceneitem_t *item);

/** Only for rendering*/
EXPORT bool pls_vertical_canvas_is_rendering();

/** Only for rendering*/
EXPORT bool pls_landscape_canvas_is_rendering();

/** Create dual output scene **/
EXPORT obs_scene_t *pls_create_vertical_scene(const char *name);

/** Enum scenes list (including vertical scenes)**/
EXPORT void pls_enum_all_scenes(bool (*enum_proc)(void *, obs_source_t *),
				void *param);

/** Check if a scene is used for dual output*/
EXPORT bool pls_is_vertical_scene(obs_scene_t *scene);

EXPORT bool pls_obs_is_rendering();

#ifdef __cplusplus
}
#endif
