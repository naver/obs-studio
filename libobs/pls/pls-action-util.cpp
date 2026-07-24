#ifdef PLS_UI_ACTION_STATS
#include "pls-action-util.h"
#include "pls-base.h"
#include "util/platform.h"
#include <util/c99defs.h>
#include "util/threading.h"
#include "obs.h"
#include <string>
#include <mutex>
#include <assert.h>

THREAD_LOCAL bool is_setting_updated_delay = false;
THREAD_LOCAL obs_source_t *current_source = NULL;

struct source_action_helper {
	std::mutex autolock;
	int propert_flag = 0;
	std::string property_field;
	uint64_t update_time_ns = PROPERTY_NO_CHANGE;
};

struct sceneitem_action_helper {
	std::mutex autolock;
	int propert_flag = 0;
	std::string property_field;
};

struct scene_action_helper {
	std::mutex autolock;
	int propert_flag = 0;
	std::string property_field;
};

void *pls_create_action_helper(enum ACTION_HELPER_TYPE type)
{
	switch (type) {
	case ACTION_HELPER_TYPE_SOURCE:
		return new source_action_helper();
	case ACTION_HELPER_TYPE_SCENEITEM:
		return new sceneitem_action_helper();
	case ACTION_HELPER_TYPE_SCENE:
		return new scene_action_helper();
	default:
		assert(false);
		return nullptr;
	}
}

void pls_free_action_helper(void *ptr, enum ACTION_HELPER_TYPE type)
{
	if (!ptr)
		return;

	switch (type) {
	case ACTION_HELPER_TYPE_SOURCE:
		delete static_cast<source_action_helper *>(ptr);
		break;

	case ACTION_HELPER_TYPE_SCENEITEM:
		delete static_cast<sceneitem_action_helper *>(ptr);
		break;

	case ACTION_HELPER_TYPE_SCENE:
		delete static_cast<scene_action_helper *>(ptr);
		break;
	default:
		assert(false);
		break;
	}
}

source_action_helper *get_action_helper(obs_source_t *source)
{
	if (!source)
		return nullptr;

	auto ptr = pls_get_source_action_helper(source);
	if (!ptr)
		return nullptr;

	return static_cast<source_action_helper *>(ptr);
}

sceneitem_action_helper *get_action_helper(obs_sceneitem_t *item)
{
	if (!item)
		return nullptr;

	auto ptr = pls_get_item_action_helper(item);
	if (!ptr)
		return nullptr;

	return static_cast<sceneitem_action_helper *>(ptr);
}

scene_action_helper *get_action_helper(obs_scene *item)
{
	if (!item)
		return nullptr;

	auto ptr = pls_get_scene_action_helper(item);
	if (!ptr)
		return nullptr;

	return static_cast<scene_action_helper *>(ptr);
}

void pls_set_current_source(obs_source_t *source)
{
	current_source = source;
}

obs_source_t *pls_get_current_source()
{
	return current_source;
}

void pls_set_property_update_delay(bool delay_update)
{
	is_setting_updated_delay = delay_update;
}

bool pls_is_property_update_delay() // return delay_update
{
	return is_setting_updated_delay;
}

void pls_on_source_property_changed(obs_source_t *source, const char *property_name)
{
	if (!source || !property_name)
		return;

	auto helper = get_action_helper(source);
	if (!helper)
		return;

	{
		std::unique_lock<std::mutex> lock(helper->autolock);
		helper->propert_flag = PROPERTY_UI_CHANGED;
		helper->property_field = property_name;
	}

	// "text_gdiplus:vertical property will be changed in the ui"
	PLS_UI_ACTION_OBS("%s:%s property will be changed in the ui", obs_source_get_id(source),
			  helper->property_field.c_str());
}

void pls_on_source_property_updated(obs_source_t *source)
{
	if (!source)
		return;

	obs_source_t *action_parent = pls_get_action_parent(source);

	obs_source_t *real_source = action_parent ? action_parent : source;

	auto helper = get_action_helper(real_source);
	if (!helper)
		return;

	std::unique_lock<std::mutex> lock(helper->autolock);
	if (PROPERTY_UI_CHANGED == helper->propert_flag) {
		helper->propert_flag = PROPERTY_UPDATED;
		helper->update_time_ns = os_gettime_ns();
	}
}

void check_source_property_update_interval(obs_source_t *source, uint64_t timeout_ns, source_action_helper *helper)
{
	if (!source || !helper)
		return;

	bool is_action = false;

	do {
		std::unique_lock<std::mutex> lock(helper->autolock);
		if (PROPERTY_UPDATED != helper->propert_flag)
			break;

		auto now_tm = os_gettime_ns();

		auto type = obs_source_get_type(source);
		if (type == OBS_SOURCE_TYPE_FILTER || type == OBS_SOURCE_TYPE_TRANSITION) {
			if (now_tm - helper->update_time_ns >= PROPERTY_RENDER_TIMEOUT) {
				helper->propert_flag = PROPERTY_NO_CHANGE;
				break; // if no captured data, transition/filter will not be rendered, we need ignore this case
			}
		}

		if (timeout_ns > 0 && now_tm - helper->update_time_ns >= timeout_ns) {
			helper->propert_flag = PROPERTY_NO_CHANGE;
			break;
		}

		helper->propert_flag = PROPERTY_NO_CHANGE;
		is_action = true;

	} while (false);

	if (is_action) {
		// "text_gdiplus:vertical property is applied in render"
		PLS_UI_ACTION_OBS("%s:%s property is applied in render", obs_source_get_id(source),
				  helper->property_field.c_str());
	}
}

void pls_on_source_property_render(obs_source_t *source, uint64_t timeout_ns)
{
	if (!source)
		return;

	obs_source_t *action_parent = pls_get_action_parent(source);

	obs_source_t *real_source = action_parent ? action_parent : source;

	auto helper = get_action_helper(real_source);
	if (!helper)
		return;

	check_source_property_update_interval(real_source, timeout_ns, helper);
	// check other interval
}

std::string get_sceneitem_property_field(enum SCENEITEM_ATTRIBUTE_TYPE type)
{
	switch (type) {
	case HIDE_ITEM:
		return "hide";
	case SHOW_ITEM:
		return "show";
	case MOVE_ITEM:
		return "move";
	case ROTATE_ITEM:
		return "rotate";
	case SCALE_ITEM:
		return "scale";
	case CROP_ITEM:
		return "crop";
	case FIT_STRETCH_TO_SCREEN:
		return "fit_stretch_toscreen";
	case CENTER_TO_SCREEN:
		return "center_toscreen";
	case FLIP_H_V:
		return "flip_h_v";
	case BELEND_MODE:
		return "blend_mode";
	case SCALE_FILTER:
		return "scale_filter";
	case MOVE_ORDER:
		return "move_order";
	case BELEND_METHOD:
		return "belend_method";
	case EDIT_TRANSFORM:
		return "edit_transform";
	case RESET_TRANSFORM:
		return "reset_transform"; 
	default:
		assert(false);
		return "";
	}
}

void pls_on_item_attribute_changed(obs_sceneitem_t *item, enum SCENEITEM_ATTRIBUTE_TYPE type, int step)
{
	if (!item)
		return;

	auto helper = get_action_helper(item);
	if (!helper)
		return;

	auto str = get_sceneitem_property_field(type);
	if (str.empty())
		return;

	std::unique_lock<std::mutex> lock(helper->autolock);
	if (step == PROPERTY_UI_CHANGED) {
		helper->propert_flag = step;
		helper->property_field = str;
		PLS_UI_ACTION_OBS("[%s] sceneitem is changed", helper->property_field.c_str());
	} else if (step == PROPERTY_UPDATED) {
		if (helper->propert_flag == PROPERTY_UI_CHANGED) {
			helper->propert_flag = step;
		}
	} else {
		assert(false);
		return;
	}
}

void pls_on_item_render(obs_sceneitem_t *item)
{
	if (!item)
		return;

	auto helper = get_action_helper(item);
	if (!helper)
		return;

	std::unique_lock<std::mutex> lock(helper->autolock);
	if (!helper->property_field.empty() && helper->propert_flag == PROPERTY_UPDATED) {
		PLS_UI_ACTION_OBS("[%s] sceneitem is applied", helper->property_field.c_str());
		helper->propert_flag = PROPERTY_NO_CHANGE;
		helper->property_field = "";
	}
}

void pls_on_drawpen_event(obs_scene_t *scene, const char *operation)
{
	if (!scene || !operation)
		return;

	auto helper = get_action_helper(scene);
	if (!helper)
		return;

	{
		std::unique_lock<std::mutex> lock(helper->autolock);
		helper->propert_flag = PROPERTY_UI_CHANGED;
		helper->property_field = operation;
	}

	PLS_UI_ACTION_OBS("[%s] drawpen event start", operation);
}

void pls_on_drawpen_updated(obs_scene_t *scene)
{
	if (!scene)
		return;

	auto helper = get_action_helper(scene);
	if (!helper)
		return;

	std::unique_lock<std::mutex> lock(helper->autolock);
	if (PROPERTY_UI_CHANGED == helper->propert_flag) {
		helper->propert_flag = PROPERTY_UPDATED;
	}
}

void pls_on_drawpen_render(obs_scene_t *scene)
{
	if (!scene)
		return;

	auto helper = get_action_helper(scene);
	if (!helper)
		return;

	bool is_action = false;
	std::string name;

	{
		std::unique_lock<std::mutex> lock(helper->autolock);
		if (PROPERTY_UPDATED == helper->propert_flag && !helper->property_field.empty()) {
			helper->propert_flag = PROPERTY_NO_CHANGE;
			name = helper->property_field;
			is_action = true;
		}
	}

	if (is_action) {
		PLS_UI_ACTION_OBS("[%s] drawpen event completed", name.c_str());
	}
}

#endif // PLS_UI_ACTION_STATS
