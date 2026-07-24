#include "obs-internal.h"
#include "pls-dual-output.h"
#include "pls-dual-output-internal.h"
#include "obs-scene.h"

THREAD_LOCAL bool is_save_bypass_vertical = true;
THREAD_LOCAL bool is_add_vertical_scene_item = false;
THREAD_LOCAL bool is_dual_output_current_check_on = true;
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

	struct obs_video_info_v2 *vertical_canvas = obs_create_vertical_video_info_v2();

	obs->video.dual_output_canvas_created = true;

	int ret = obs_set_video_info(vertical_canvas, ovi, false);
	if (ret != OBS_VIDEO_SUCCESS)
		return NULL;

	video_t *video = pls_get_vertical_video_t();

	if (video != NULL)
		obs->video.dual_output_initialized = true;
	blog(LOG_INFO, "%s [Exit]: ovi=%p, video=%p", __FUNCTION__, ovi, video);
	return video;
}

int pls_vertical_view_remove()
{
	blog(LOG_INFO, "%s [Enter]", __FUNCTION__);
	struct obs_video_info_v2 *vertical_canvas = obs_get_canvas_by_index(VERTIVAL_CANVAS_INDEX);
	assert(vertical_canvas != NULL);

	int ret = obs_remove_video_info(vertical_canvas, false);
	if (ret != OBS_VIDEO_SUCCESS) {
		blog(LOG_INFO, "%s [Exit]: ret=[%d]", __FUNCTION__, ret);
		return ret;
	}

	obs->video.dual_output_initialized = false;

	blog(LOG_INFO, "%s [Exit]", __FUNCTION__);

	return ret;
}

void pls_render_vertical_main_texture_src_color_only()
{
	struct obs_core_video_mix *mix = pls_get_vertical_mix();
	assert(mix != NULL);
	if (mix != NULL) {
		obs_render_main_texture_internal(GS_BLEND_ONE, GS_BLEND_ZERO, GS_BLEND_ONE, GS_BLEND_INVSRCALPHA, mix);
	}
}

void pls_render_vertical_main_texture(void)
{
	struct obs_core_video_mix *mix = pls_get_vertical_mix();
	assert(mix != NULL);
	if (mix != NULL) {
		obs_render_main_texture_internal(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA, GS_BLEND_ONE, GS_BLEND_INVSRCALPHA,
						 mix);
	}
}

void pls_source_video_render_vertical_when_render_display(obs_source_t *source)
{ 
	struct obs_video_info *old_ovi = obs_get_video_rendering_canvas();
	obs_set_video_rendering_canvas(obs_get_canvas_by_index(VERTIVAL_CANVAS_INDEX)->ovi); 
	pls_source_video_render_vertical(source);
	obs_set_video_rendering_canvas(old_ovi);
}

void pls_source_video_render_vertical(obs_source_t *source)
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
	if (obs == NULL)
		return false;
	return obs->video.dual_output_on;
}

void pls_set_dual_output_on(bool is_on)
{
	blog(LOG_INFO, "%s: is_on=%d", __FUNCTION__, is_on);
	obs->video.dual_output_on = is_on;
}

bool pls_is_dual_output_initialized()
{
	if (obs == NULL)
		return false;
	return obs->video.dual_output_initialized;
}

struct obs_core_video_mix *pls_get_vertical_mix()
{
	return obs->video.vertical_mix;
}

video_t *pls_get_vertical_video_t()
{
	struct obs_core_video_mix *mix = pls_get_vertical_mix();
	return mix != NULL ? mix->video : NULL;
}

bool pls_get_vertical_video_info(struct obs_video_info *ovi)
{
	if (ovi == NULL) {
		return false;
	}

	struct obs_video_info_v2 *vertical_canvas = obs_get_canvas_by_index(VERTIVAL_CANVAS_INDEX);
	if (vertical_canvas == NULL) {
		return false;
	}

	*ovi = *(vertical_canvas->ovi);
	return true;
}

obs_sceneitem_t *pls_vertical_scene_add(obs_scene_t *scene, obs_source_t *source, obs_sceneitem_t *insert_after,
					obs_data_t *settings)
{
	blog(LOG_INFO, "%s: scene=%p, source=%p, insert_after=%p, settings=%p", __FUNCTION__, scene, source,
	     insert_after, settings);
	return obs_scene_add_vertical(scene, source, insert_after, settings);
}

bool pls_is_vertical_sceneitem(obs_sceneitem_t *item)
{
	if (item == NULL) {
		return false;
	}
	return item->is_vertical;
}

obs_sceneitem_t *pls_vertical_sceneitem_get_parent_group(obs_scene_t *scene_v, obs_sceneitem_t *group_subitem_v)
{
	if (!scene_v->is_vertical || !group_subitem_v->is_vertical)
		return NULL;

	if (group_subitem_v->is_group)
		return NULL;

	obs_sceneitem_t *item = scene_v->first_item;
	while (item) {
		if (item->is_group && item->source->context.data == group_subitem_v->parent)
			return item;
		item = item->next;
	}

	return NULL;
}

bool pls_is_vertical_scene_or_group(obs_source_t *source)
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

obs_data_array_t *pls_save_sources_filtered_all(obs_save_source_filter_cb cb, void *data_)
{
	is_save_bypass_vertical = false;
	obs_data_array_t *ret = obs_save_sources_filtered(cb, data_);
	is_save_bypass_vertical = true;
	return ret;
}

obs_data_t *pls_save_source_smart(obs_source_t *source, bool bypass_vertical)
{
	is_save_bypass_vertical = bypass_vertical;
	obs_data_t *ret = obs_save_source(source);
	is_save_bypass_vertical = true;
	return ret;
}

void pls_scene_enum_items_all(obs_scene_t *scene, bool (*callback)(obs_scene_t *, obs_sceneitem_t *, void *),
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
	obs_data_t *ret = obs_scene_save_transform_states(scene, all_items);
	is_enum_scene_include_vertical--;
	assert(is_enum_scene_include_vertical >= 0);
	return ret;
}

void pls_sceneitem_group_enum_items_all(obs_sceneitem_t *group,
					bool (*callback)(obs_scene_t *, obs_sceneitem_t *, void *), void *param)
{
	if (!group || !group->is_group)
		return;

	obs_scene_t *scene = group->source->context.data;
	if (scene)
		pls_scene_enum_items_all(scene, callback, param);
}

void pls_enum_hotkeys_all(obs_hotkey_enum_func func, void *data)
{
	is_enum_hotkey_bypass_vertical = false;
	obs_enum_hotkeys(func, data);
	is_enum_hotkey_bypass_vertical = true;
}

void pls_enum_hotkey_bindings_all(obs_hotkey_binding_enum_func func, void *data)
{
	obs_enum_hotkey_bindings_all(func, data);
}

void pls_obs_hotkey_load_bindings(obs_hotkey_id id, obs_key_combination_t *combinations, size_t num)
{
	is_enum_hotkey_bypass_vertical = false;
	obs_hotkey_load_bindings(id, combinations, num);
	is_enum_hotkey_bypass_vertical = true;
}

bool pls_is_vertical_hotkey(obs_hotkey_t *hotkey)
{
	if (hotkey == NULL) {
		return false;
	}
	return hotkey->is_vertical;
}

bool pls_is_vertical_hotkey_id(obs_hotkey_id id)
{
	obs_hotkey_t *hotkey = obs_hotkey_get_by_id(id);
	if (hotkey == NULL) {
		return false;
	}
	return pls_is_vertical_hotkey(hotkey);
}

bool pls_is_sceneitem_hotkey(obs_hotkey_t *hotkey)
{
	if (hotkey == NULL) {
		return false;
	}
	return hotkey->is_sceneitem;
}

bool pls_is_sceneitem_hotkey_id(obs_hotkey_id id)
{
	obs_hotkey_t *hotkey = obs_hotkey_get_by_id(id);
	if (hotkey == NULL) {
		return false;
	}
	return pls_is_sceneitem_hotkey(hotkey);
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

void pls_set_dual_output_current_check_on()
{
	is_dual_output_current_check_on = true;
}

void pls_set_dual_output_current_check_off()
{
	is_dual_output_current_check_on = false;
}

static bool ovi_is_canvas(struct obs_video_info_v2 *ovi_v2, struct obs_video_info *canvas)
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

	struct obs_video_info *current_canvas = obs_get_video_rendering_canvas();
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
	if (is_force_render_vertical > 0) {
		return item->is_vertical;
	}
	return false;
}

bool pls_is_vertical_ovi(struct obs_video_info_v2 *ovi_v2)
{
	if (ovi_v2 == NULL) {
		return false;
	}
	struct obs_video_info_v2 *vertical_canvas = obs_get_canvas_by_index(VERTIVAL_CANVAS_INDEX);
	if (vertical_canvas == NULL) {
		return false;
	}
	return ovi_is_canvas(ovi_v2, vertical_canvas->ovi);
}

bool pls_is_same_canvas(struct obs_video_info_v2 *ovi_v2_a, struct obs_video_info_v2 *ovi_v2_b)
{
	if (ovi_v2_a == NULL || ovi_v2_b == NULL) {
		return false;
	}
	bool a_is_vertical = pls_is_vertical_ovi(ovi_v2_a);
	bool b_is_vertical = pls_is_vertical_ovi(ovi_v2_b);
	return (a_is_vertical && b_is_vertical) || (!a_is_vertical && !b_is_vertical);
}

static bool pls_canvas_is_rendering(size_t canvas_index)
{
	if (pls_obs_is_rendering()) {
		struct obs_video_info_v2 *canvas = obs_get_canvas_by_index(canvas_index);
		if (canvas == NULL) {
			return false;
		}
		return ovi_is_canvas(canvas, obs_get_video_rendering_canvas());
	}

	if (canvas_index == LANDSCAPE_CANVAS_INDEX && is_force_render_landscape > 0) {
		return true;
	}

	if (canvas_index == VERTIVAL_CANVAS_INDEX && is_force_render_vertical > 0) {
		return true;
	}

	if (is_dual_output_current_check_on) {
		assert(false);
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
			if (source->info.type == OBS_SOURCE_TYPE_SCENE && !enum_proc(param, s)) {
				obs_source_release(s);
				break;
			}
			obs_source_release(s);
		}

		source = (obs_source_t *)source->context.hh.next;
	}

	pthread_mutex_unlock(&obs->data.sources_mutex);
}

void pls_enum_vertical_scenes(bool (*enum_proc)(void *, obs_source_t *), void *param)
{
	obs_source_t *source;

	pthread_mutex_lock(&obs->data.sources_mutex);

	source = obs->data.public_sources;
	while (source) {
		obs_source_t *s = obs_source_get_ref(source);
		if (s) {
			obs_data_t *settings = obs_source_get_settings(s);
			bool is_vertical = obs_data_get_bool(settings, "is_vertical");
			obs_data_release(settings);
			if (source->info.type == OBS_SOURCE_TYPE_SCENE && is_vertical && !enum_proc(param, s)) {
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

bool pls_is_vertical_scene2(obs_source_t *source)
{
	if (source == NULL) {
		return false;
	}

	obs_scene_t *scene = obs_scene_from_source(source);

	return pls_is_vertical_scene(scene);
}

bool pls_is_vertical_group(obs_sceneitem_t *item)
{
	if (item == NULL || item->source == NULL) {
		return false;
	}

	obs_scene_t *group = obs_group_from_source(item->source);
	if (group == NULL) {
		return false;
	}

	return pls_is_vertical_scene(item->parent);
}

obs_scene_t *pls_create_vertical_scene(const char *name, bool is_group_ref)
{
	obs_data_t *settings = obs_data_create();
	obs_data_set_bool(settings, ID_IS_VERTICAL, true);
	struct obs_source *source = obs_source_create("scene", name, settings, NULL);
	obs_scene_t *scene = obs_scene_from_source(source);
	if (scene == NULL) {
		blog(LOG_INFO, "pls_create_vertical_scene: get scene failed! source=[%p]", source);
	} else {
		scene->id_counter = 10000000LL;
		if (is_group_ref)
			scene->is_vertical_group_ref = true;
	}
	return source->context.data;
}

void pls_vertical_scene_set_group_ref(obs_scene_t *scene, bool is_group_ref)
{
	if (scene)
		scene->is_vertical_group_ref = is_group_ref;
}

bool pls_bind_vertical_scene(obs_scene_t *real_scene, obs_scene_t *fake_scene)
{
	if (!fake_scene) {
		return false;
	}
	fake_scene->real_scene = real_scene;
	return true;
}

obs_scene_t *pls_vertical_scene_get_real_scene(obs_scene_t *scene)
{
	if (!scene || !scene->is_vertical) {
		return NULL;
	}
	return scene->real_scene;
}

/** only for core */
//PRISM/chenguoxi/20241104/PRISM_PC-1452/dual output
struct obs_video_info_v2 *obs_get_canvas_by_index(size_t index)
{
	// blog(LOG_INFO, "[VIDEO_CANVAS] get canvas by index %zu", index);
	if (index >= obs->video.canvases.num)
		return NULL;

	//PRISM/chenguoxi/20250313/PRISM_PC_NELO-240/ensure vertical canvas exists
	// There are two key points:
	// 1. The landscape canvas is created when prism starts. So it always exists.
	// 2. The vertical canvas is available only when dual output canvas is created.
	if (!obs->video.dual_output_canvas_created && index > LANDSCAPE_CANVAS_INDEX) {
		return NULL;
	}

	return obs->video.canvases.array[index];
}

struct obs_video_info *pls_get_real_canvas(struct obs_video_info_v2 *ovi_v2)
{
	if (ovi_v2 == NULL) {
		return NULL;
	}
	struct obs_video_info_v2 *real_canvas_v2 = ovi_v2;
	while (real_canvas_v2->parent != NULL) {
		real_canvas_v2 = real_canvas_v2->parent;
	}
	return real_canvas_v2->ovi;
}
