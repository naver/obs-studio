#pragma once

#include "obs.h"

/**  only for obs core */

#ifdef __cplusplus
extern "C" {
#endif

#define VERTIVAL_CANVAS_INDEX 1
#define LANDSCAPE_CANVAS_INDEX 0

extern THREAD_LOCAL bool is_save_bypass_vertical;
extern THREAD_LOCAL bool is_add_vertical_scene_item;
extern THREAD_LOCAL bool is_dual_output_current_check_on;
extern THREAD_LOCAL int is_enum_scene_include_vertical;
extern THREAD_LOCAL bool is_enum_hotkey_bypass_vertical;
extern THREAD_LOCAL int is_force_render_vertical;
extern THREAD_LOCAL int is_force_render_landscape;
extern THREAD_LOCAL int is_force_get_vertical_size;
extern THREAD_LOCAL bool is_rendering;

//PRISM/chenguoxi/20241104/PRISM_PC-1452/dual output
struct obs_video_info_v2 {
	struct obs_video_info *ovi;
	bool initialized;
	struct obs_video_info_v2 *parent;
};

extern obs_hotkey_t *obs_hotkey_get_by_id(obs_hotkey_id id);
extern obs_hotkey_pair_t *obs_hotkey_get_pair_by_id(obs_hotkey_pair_id id);
extern bool obs_hotkey_set_vertical(obs_hotkey_id id);
extern bool obs_hotkey_pair_set_vertical(obs_hotkey_pair_id id);
extern bool obs_hotkey_set_is_sceneitem(obs_hotkey_id id);
extern bool obs_hotkey_pair_set_is_sceneitem(obs_hotkey_pair_id id);
extern void obs_enum_hotkey_bindings_all(obs_hotkey_binding_enum_func func, void *data);

/** Sets/Gets current video rendering canvas*/
extern void obs_set_video_rendering_canvas(struct obs_video_info *ovi);
extern struct obs_video_info *obs_get_video_rendering_canvas(void);

//PRISM/chenguoxi/20241104/PRISM_PC-1452/dual output
extern struct obs_video_info_v2 *obs_get_canvas_by_index(size_t index);
//PRISM/chenguoxi/20241104/PRISM_PC-1452/dual output
extern struct obs_video_info_v2 *obs_sceneitem_get_canvas(obs_sceneitem_t *item);
//PRISM/chenguoxi/20241104/PRISM_PC-1452/dual output
extern void scene_save_internal(void *data, obs_data_t *settings);
//PRISM/chenguoxi/20241104/PRISM_PC-1452/dual output
extern void obs_render_main_texture_internal(enum gs_blend_type src_c, enum gs_blend_type dest_c,
					     enum gs_blend_type src_a, enum gs_blend_type dest_a,
					     struct obs_core_video_mix *video);
//PRISM/chenguoxi/20241104/PRISM_PC-1452/dual output
extern void obs_transition_recalculate_size(obs_source_t *transition);
//PRISM/chenguoxi/20241104/PRISM_PC-1452/dual output
extern struct obs_video_info_v2 *obs_create_3dr_ovi(struct obs_video_info *in);

extern struct obs_video_info_v2 *obs_find_ovi_v2_by_ovi(struct obs_video_info *ovi);

/** Remove a video info */
extern int obs_remove_video_info(struct obs_video_info_v2 *ovi, bool start_graphics_thread);
/** Adds new video info to array of video info objects, need to be initialized */
extern struct obs_video_info_v2 *obs_create_landscape_video_info_v2();
extern struct obs_video_info_v2 *obs_create_vertical_video_info_v2();
extern int obs_set_video_info(struct obs_video_info_v2 *canvas, struct obs_video_info *updated,
			      bool start_graphics_thread);
extern int obs_deactivate_video_info();

extern struct obs_video_info *pls_get_real_canvas(struct obs_video_info_v2 *ovi_v2);

#ifdef __cplusplus
}
#endif
