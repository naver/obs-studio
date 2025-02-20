#include "obs-internal.h"
#include "pls-dual-output.h"
#include "pls-dual-output-internal.h"
#include "obs-scene.h"

THREAD_LOCAL bool is_save_bypass_vertical = true;
THREAD_LOCAL int is_enum_scene_include_vertical = 0;
THREAD_LOCAL bool is_enum_hotkey_bypass_vertical = true;
THREAD_LOCAL int is_force_render_vertical = 0;
THREAD_LOCAL int is_force_render_landscape = 0;
THREAD_LOCAL int is_force_get_vertical_size = 0;

THREAD_LOCAL bool is_rendering = false;

video_t *pls_vertical_view_add(struct obs_video_info *ovi)
{
	blog(LOG_INFO, "%s [Enter]: ovi=%p", __FUNCTION__, ovi);

	if (ovi == NULL)
		return NULL;

	struct obs_video_info_v2 *vertical_canvas =
		obs_get_canvas_by_index(VERTIVAL_CANVAS_INDEX);
	assert(vertical_canvas == NULL);
	if (vertical_canvas == NULL) {
		vertical_canvas = obs_create_video_info_v2();
	}

	int ret = obs_set_video_info(vertical_canvas, ovi);
	if (ret != OBS_VIDEO_SUCCESS)
		return NULL;

	video_t *video = pls_get_vertical_video_t();

	if (video != NULL)
		obs->video.dual_output_initialized = true;
	blog(LOG_INFO, "%s [Exit]: ovi=%p, video=%p", __FUNCTION__, ovi, video);
	return video;
}

void pls_vertical_view_remove()
{
	blog(LOG_INFO, "%s [Enter]", __FUNCTION__);
	struct obs_video_info_v2 *vertical_canvas =
		obs_get_canvas_by_index(VERTIVAL_CANVAS_INDEX);
	assert(vertical_canvas != NULL);
	
	obs_remove_video_info(vertical_canvas);
	obs->video.dual_output_initialized = false;

	blog(LOG_INFO, "%s [Exit]", __FUNCTION__);
}

void pls_render_vertical_main_texture_src_color_only()
{
	struct obs_core_video_mix *mix = pls_get_vertical_mix();
	assert(mix != NULL);
	if (mix != NULL) {
		obs_render_main_texture_internal(GS_BLEND_ONE, GS_BLEND_ZERO,
						 GS_BLEND_ONE,
						 GS_BLEND_INVSRCALPHA, mix);
	}
}

void pls_render_vertical_main_texture(void)
{
	struct obs_core_video_mix *mix = pls_get_vertical_mix();
	assert(mix != NULL);
	if (mix != NULL) {
		obs_render_main_texture_internal(
			GS_BLEND_ONE, GS_BLEND_INVSRCALPHA, GS_BLEND_ONE,
			GS_BLEND_INVSRCALPHA, mix);
	}
}

void pls_source_video_render_vertical(obs_source_t* source)
{
	is_force_render_vertical++;
	obs_source_video_render(source);
	is_force_render_vertical--;
	assert(is_force_render_vertical >= 0);
}

void pls_source_video_render_landscape(obs_source_t *source)
{
	is_force_render_landscape++;
	obs_source_video_render(source);
	is_force_render_landscape--;
	assert(is_force_render_landscape >= 0);
}


bool pls_is_dual_output_on()
{
	return obs->video.dual_output_on;
}

void pls_set_dual_output_on(bool is_on)
{
	blog(LOG_INFO, "%s: is_on=%d", __FUNCTION__, is_on);
	obs->video.dual_output_on = is_on;
}

bool pls_is_dual_output_initialized()
{
	return obs->video.dual_output_initialized;
}

struct obs_core_video_mix* pls_get_vertical_mix()
{
	struct obs_video_info_v2 *vertical_canvas =
		obs_get_canvas_by_index(VERTIVAL_CANVAS_INDEX);
	if (vertical_canvas == NULL) {
		return NULL;
	}

	struct obs_core_video_mix *vertical_mix = NULL;

	pthread_mutex_lock(&obs->video.mixes_mutex);
	for (size_t i = 0; i < obs->video.mixes.num; i++) {
		struct obs_core_video_mix *mix = obs->video.mixes.array[i];
		if (mix->ovi == vertical_canvas->ovi) {
			vertical_mix = mix;
			break;
		}
	}
	pthread_mutex_unlock(&obs->video.mixes_mutex);

	return vertical_mix;
}

video_t* pls_get_vertical_video_t()
{
	struct obs_core_video_mix *mix = pls_get_vertical_mix();
	return mix != NULL ? mix->video : NULL;
}

bool pls_get_vertical_video_info(struct obs_video_info *ovi)
{
	if (ovi == NULL) {
		return false;
	}

	struct obs_video_info_v2 *vertical_canvas =
		obs_get_canvas_by_index(VERTIVAL_CANVAS_INDEX);
	if (vertical_canvas == NULL) {
		return false;
	}

	*ovi = *(vertical_canvas->ovi);
	return true;
}

obs_sceneitem_t *pls_vertical_scene_add(obs_scene_t *scene,
					obs_source_t *source,
					obs_sceneitem_t *insert_after,
					obs_data_t *settings)
{
	blog(LOG_INFO, "%s: scene=%p, source=%p, insert_after=%p, settings=%p", __FUNCTION__, scene, source, insert_after, settings);
	return obs_scene_add_vertical(scene, source, insert_after, settings);
}

bool pls_is_vertical_sceneitem(obs_sceneitem_t *item)
{
	if (item == NULL) {
		return false;
	}
	return item->is_vertical;
}

bool pls_is_vertical_scene_or_group(obs_source_t* source)
{
	obs_scene_t *scene = obs_scene_from_source(source);
	if (scene != NULL) {
		return scene->is_vertical;
	}

	scene = obs_group_from_source(source);
	if (scene != NULL) {
		return scene->is_vertical;
	}

	return false;
}

obs_data_array_t *pls_save_sources_filtered_all(obs_save_source_filter_cb cb,
						void *data_)
{
	is_save_bypass_vertical = false;
	obs_data_array_t* ret = obs_save_sources_filtered(cb, data_);
	is_save_bypass_vertical = true;
	return ret;
}

obs_data_t *pls_save_source_smart(obs_source_t *source, bool bypass_vertical)
{
	is_save_bypass_vertical = bypass_vertical;
	obs_data_t* ret = obs_save_source(source);
	is_save_bypass_vertical = true;
	return ret;
}

void pls_scene_enum_items_all(obs_scene_t *scene,
			      bool (*callback)(obs_scene_t *, obs_sceneitem_t *,
					       void *),
			      void *param)
{
	is_enum_scene_include_vertical++;
	obs_scene_enum_items(scene, callback, param);
	is_enum_scene_include_vertical--;
	assert(is_enum_scene_include_vertical >= 0);
}

obs_data_t *pls_scene_save_transform_states_all(obs_scene_t *scene, bool all_items)
{
	is_enum_scene_include_vertical++;
	obs_data_t* ret = obs_scene_save_transform_states(scene, all_items);
	is_enum_scene_include_vertical--;
	assert(is_enum_scene_include_vertical >= 0);
	return ret;
}

void pls_sceneitem_group_enum_items_all(obs_sceneitem_t *group,
				    bool (*callback)(obs_scene_t *,
						     obs_sceneitem_t *, void *),
				    void *param)
{
	if (!group || !group->is_group)
		return;

	obs_scene_t *scene = group->source->context.data;
	if (scene)
		pls_scene_enum_items_all(scene, callback, param);
}


void pls_enum_hotkeys_all(obs_hotkey_enum_func func, void* data)
{
	is_enum_hotkey_bypass_vertical = false;
	obs_enum_hotkeys(func, data);
	is_enum_hotkey_bypass_vertical = true;
}

void pls_enum_hotkey_bindings_all(obs_hotkey_binding_enum_func func,
					 void *data)
{
	obs_enum_hotkey_bindings_all(func, data);
}

bool pls_is_vertical_hotkey(obs_hotkey_t* hotkey)
{
	return hotkey->is_vertical;
}

void pls_scene_save_all(obs_scene_t *scene, obs_data_t *settings)
{
	is_save_bypass_vertical = false;
	scene_save_internal(scene, settings);
	is_save_bypass_vertical = true;
}

uint32_t pls_source_get_vertical_width(obs_source_t *source)
{
	is_force_get_vertical_size++;
	uint32_t ret = obs_source_get_width(source);
	is_force_get_vertical_size--;
	assert(is_force_get_vertical_size >= 0);
	return ret;
}

uint32_t pls_source_get_vertical_height(obs_source_t *source)
{
	is_force_get_vertical_size++;
	uint32_t ret = obs_source_get_height(source);
	is_force_get_vertical_size--;
	assert(is_force_get_vertical_size >= 0);
	return ret;
}

bool pls_get_video_info_current(struct obs_video_info *ovi)
{
	return obs_get_video_info_current(ovi);
}

static bool ovi_is_canvas(struct obs_video_info_v2* ovi_v2, struct obs_video_info* canvas)
{
	struct obs_video_info_v2 *real_canvas = ovi_v2;
	while (real_canvas != NULL) {
		if (real_canvas->ovi == canvas) {
			return true;
		}
		real_canvas = real_canvas->parent;
	}
	return false;
}

bool pls_sceneitem_is_rendering(obs_sceneitem_t *item)
{
	struct obs_video_info_v2 *item_canvas = obs_sceneitem_get_canvas(item);
	if (item_canvas == NULL)
		return false;

	struct obs_video_info *current_canvas =
		obs_get_video_rendering_canvas();
	if (current_canvas != NULL) {
		// rendering
		if (ovi_is_canvas(item_canvas, current_canvas))
			return true;		
	}

	// force rendering
	if (is_force_render_landscape > 0) {
		return !item->is_vertical;
	}

	// force rendering
	if (is_force_render_vertical > 0)
	{
		return item->is_vertical;
	}
	return false;
}

static bool pls_canvas_is_rendering(size_t canvas_index)
{
	if (pls_obs_is_rendering()) {
		struct obs_video_info_v2 *canvas =
			obs_get_canvas_by_index(canvas_index);
		if (canvas == NULL) {
			return false;
		}
		return ovi_is_canvas(canvas, obs_get_video_rendering_canvas());
	}

	if (canvas_index == LANDSCAPE_CANVAS_INDEX &&
		is_force_render_landscape > 0)
	{
		return true;
	}

	if (canvas_index == VERTIVAL_CANVAS_INDEX &&
	    is_force_render_vertical > 0) {
		return true;
	}

	return false;
}

bool pls_vertical_canvas_is_rendering()
{
	return pls_canvas_is_rendering(VERTIVAL_CANVAS_INDEX);
}

bool pls_landscape_canvas_is_rendering()
{
	return pls_canvas_is_rendering(LANDSCAPE_CANVAS_INDEX);
}

bool pls_obs_is_rendering()
{
	return is_rendering;
}

void pls_enum_all_scenes(bool (*enum_proc)(void *, obs_source_t *), void *param)
{
	obs_source_t *source;

	pthread_mutex_lock(&obs->data.sources_mutex);

	source = obs->data.public_sources;
	while (source) {
		obs_source_t *s = obs_source_get_ref(source);
		if (s) {
			if (source->info.type == OBS_SOURCE_TYPE_SCENE &&
			    !enum_proc(param, s)) {
				obs_source_release(s);
				break;
			}
			obs_source_release(s);
		}

		source = (obs_source_t *)source->context.hh.next;
	}

	pthread_mutex_unlock(&obs->data.sources_mutex);
}

bool pls_is_vertical_scene(obs_scene_t *scene)
{
	if (!scene)
		return false;

	return scene->is_vertical;
}

obs_scene_t *pls_create_vertical_scene(const char *name)
{
	obs_data_t *settings = obs_data_create();
	obs_data_set_bool(settings, ID_IS_VERTICAL, true);
	struct obs_source *source =
		obs_source_create("scene", name, settings, NULL);
	return source->context.data;
}

/** only for core */
//PRISM/chenguoxi/20241104/PRISM_PC-1452/dual output
struct obs_video_info_v2 *obs_get_canvas_by_index(size_t index)
{
	// blog(LOG_INFO, "[VIDEO_CANVAS] get canvas by index %zu", index);
	if (index >= obs->video.canvases.num)
		return NULL;
	return obs->video.canvases.array[index];
}
