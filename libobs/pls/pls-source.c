#include "pls-source.h"
#include "obs-internal.h"
#include "obs-scene.h"

static void free_pls_data(void *type_data)
{
	struct pls_source_info *pls_source_info = (struct pls_source_info *)type_data;

	bfree(pls_source_info);
}

const int IS_PLS_SOURCE = 0x63278228;

void register_pls_source_info(struct obs_source_info *obs_info, struct pls_source_info *pls_info)
{
	pls_info->is_pls_source = IS_PLS_SOURCE;
	obs_info->type_data = bmemdup(pls_info, sizeof(struct pls_source_info));
	obs_info->free_type_data = free_pls_data;
}

#define pls_source_invoke_func(FuncName, ...)                                                               \
	if (source->context.data && source->info.type_data) {                                               \
		struct pls_source_info *pls_source_info = (struct pls_source_info *)source->info.type_data; \
		if (IS_PLS_SOURCE != pls_source_info->is_pls_source) {                                      \
			return false;                                                                       \
		}                                                                                           \
		if (pls_source_info->FuncName) {                                                            \
			pls_source_info->FuncName(source->context.data, ##__VA_ARGS__);                     \
			return true;                                                                        \
		}                                                                                           \
	}                                                                                                   \
	return false;

bool pls_source_set_private_data(obs_source_t *source, obs_data_t *data)
{
	pls_source_invoke_func(set_private_data, data);
}

bool pls_source_get_private_data(obs_source_t *source, obs_data_t *data)
{
	pls_source_invoke_func(get_private_data, data);
}

bool pls_plugin_set_private_data(const char *id, obs_data_t *data)
{
	const struct obs_source_info *info = get_source_info(id);
	if (!info)
		return false;

	const struct pls_source_info *pls_source_info = (struct pls_source_info *)info->type_data;
	if (pls_source_info && pls_source_info->set_private_data) {
		pls_source_info->set_private_data(NULL, data);
		return true;
	}
	return false;
}

bool pls_plugin_get_private_data(const char *id, obs_data_t *out_data)
{
	const struct obs_source_info *info = get_source_info(id);
	if (!info)
		return false;

	const struct pls_source_info *pls_source_info = (struct pls_source_info *)info->type_data;
	if (pls_source_info && pls_source_info->get_private_data) {
		pls_source_info->get_private_data(NULL, out_data);
		return true;
	}
	return false;
}

bool pls_source_properties_edit_start(obs_source_t *source)
{
	pls_source_invoke_func(properties_edit_start, source->context.settings);
}

bool pls_source_properties_edit_end(obs_source_t *source, bool is_save_click)
{
	pls_source_invoke_func(properties_edit_end, source->context.settings, is_save_click);
}

//PRISM/Zhangdewen/20230202/#/update extern params
bool pls_source_update_extern_params(obs_source_t *source, const calldata_t *extern_params)
{
	pls_source_invoke_func(update_extern_params, extern_params);
}
//PRISM/Zhangdewen/20230202/#/update extern params, calldata_t: { cjson:const char*, sub_code:int }
bool pls_source_update_extern_params_json(obs_source_t *source, const char *cjson, int sub_code)
{
	struct calldata data;
	calldata_init(&data);

	calldata_set_string(&data, "cjson", cjson && cjson[0] ? cjson : "{}");
	calldata_set_int(&data, "sub_code", sub_code);
	bool retval = pls_source_update_extern_params(source, &data);
	calldata_free(&data);

	return retval;
}
//PRISM/Zhangdewen/20230202/#/move from //PRISM/RenJinbo/20210603/#none/timer source feature
bool pls_source_dispatch_cef_js(const obs_source_t *source, const char *event_name, const char *json_data)
{
	pls_source_invoke_func(cef_dispatch_js, event_name, json_data);
}

void pls_source_invoke_method(obs_source_t *source, const char *method)
{
	if (!source || !method)
		return;

	obs_data_t *data = obs_data_create();
	obs_data_set_string(data, "method", method);

	pls_source_set_private_data(source, data);
	obs_data_release(data);
}

AUDIO_COVER_STATUS pls_source_is_audio_has_cover(obs_source_t *source)
{
	if (!source || !source->is_audio_has_cover_ready)
		return MP_NOT_READY;
	return source->is_audio_has_cover ? MP_HAS_COVER : MP_NO_COVER;
}

//PRISM/chenguoxi/20250422/PRISM_PC-2756/delete occupied resources
bool pls_source_free_resources(obs_source_t *source)
{
	pls_source_invoke_func(free_resources);
}

//PRISM/chenguoxi/20250422/PRISM_PC-2756/delete occupied resources
bool pls_stop_ffmpeg_source(obs_source_t *source)
{
	if (source == NULL || 0 != strcmp(obs_source_get_id(source), "ffmpeg_source"))
		return false;

	struct media_action action = {
		.type = MEDIA_ACTION_FORCE_FREE,
	};

	pthread_mutex_lock(&source->media_actions_mutex);
	da_push_back(source->media_actions, &action);
	pthread_mutex_unlock(&source->media_actions_mutex);
	return true;
}

//PRISM/lizhiyong/20251013/PRISM_PC-4169/crash when create source failed
bool pls_source_context_data_valid(obs_source_t *source)
{
	if (source == NULL)
		return false;

	return source->context.data ? true : false;
}

//---------------------------------------- alive module start ----------------------------------------
pthread_mutex_t mutex_alive;
DARRAY(void *) alive_list;

pthread_mutex_t mutex_sampler_alive;
DARRAY(void *) alive_sampler_list;

void pls_init_alive_module()
{
	da_init(alive_list);
	pthread_mutex_init_value(&mutex_alive);
	pthread_mutex_init(&mutex_alive, NULL);

	da_init(alive_sampler_list);
	pthread_mutex_init_value(&mutex_sampler_alive);
	pthread_mutex_init(&mutex_sampler_alive, NULL);
}

void pls_add_alive(void *s)
{
	pthread_mutex_lock(&mutex_alive);
	da_push_back(alive_list, &s);
	pthread_mutex_unlock(&mutex_alive);
}

void pls_remove_alive(void *s)
{
	pthread_mutex_lock(&mutex_alive);
	da_erase_item(alive_list, &s);
	pthread_mutex_unlock(&mutex_alive);
}

bool pls_is_alive(void *s)
{
	bool alive = false;

	pthread_mutex_lock(&mutex_alive);
	for (size_t i = 0; i < alive_list.num; i++) {
		void *item = alive_list.array[i];
		if (item == s) {
			alive = true;
			break;
		}
	}
	pthread_mutex_unlock(&mutex_alive);

	return alive;
}

void pls_add_sampler_alive(void *s)
{
	pthread_mutex_lock(&mutex_sampler_alive);
	da_push_back(alive_sampler_list, &s);
	pthread_mutex_unlock(&mutex_sampler_alive);
}

void pls_remove_sampler_alive(void *s)
{
	pthread_mutex_lock(&mutex_sampler_alive);
	da_erase_item(alive_sampler_list, &s);
	pthread_mutex_unlock(&mutex_sampler_alive);
}

bool pls_sampler_is_alive(void *s)
{
	bool alive = false;

	pthread_mutex_lock(&mutex_sampler_alive);
	for (size_t i = 0; i < alive_sampler_list.num; i++) {
		void *item = alive_sampler_list.array[i];
		if (item == s) {
			alive = true;
			break;
		}
	}
	pthread_mutex_unlock(&mutex_sampler_alive);

	return alive;
}

void pls_update_source_loading(obs_source_t *source, bool loading)
{
	struct calldata data;
	uint8_t stack[128];
	bool prev_loading;

	if (!source)
		return;

	prev_loading = os_atomic_load_bool(&source->is_source_loading);
	if (prev_loading == loading)
		return;

	os_atomic_store_bool(&source->is_source_loading, loading);
	os_atomic_set_long(&source->loading_start_ms, loading ? (long)(os_gettime_ns() / 1000000) : 0);

	calldata_init_fixed(&data, stack, sizeof(stack));
	calldata_set_ptr(&data, "source", source);
	calldata_set_bool(&data, "loading", loading);
	signal_handler_signal(obs_get_signal_handler(), "source_loading", &data);
}

bool pls_is_source_loading(obs_source_t *source)
{
	return source && os_atomic_load_bool(&source->is_source_loading);
}

void pls_check_end_loading(obs_source_t *source)
{
	if (!source)
		return;

	if (!pls_is_source_loading(source))
		return;

	static const long load_timeout = 3000; // in ms

	long start = os_atomic_load_long(&source->loading_start_ms);
	long now = (long)(os_gettime_ns() / 1000000);
	if (now > start && (now - start) > load_timeout) {
		pls_update_source_loading(source, false);
	}
}

bool obs_source_check_settings_ex(obs_source_t *source, obs_data_t *output)
{
	pls_source_invoke_func(check_obs_source_settings, output);
}

//---------------------------------------- alive module end ----------------------------------------

//PRISM/FanZirong/20251125/PRISM_PC-4540/avoid reactivating DShow device for non-key param changes -start
bool pls_check_key_params_changed(obs_data_t *new_settings, obs_data_t *old_settings,
				  const struct pls_key_param_config *key_params, size_t key_params_count,
				  bool (*special_param_func)(obs_data_t *old_settings, obs_data_t *new_settings))
{
	if (!old_settings || !new_settings) {
		return true;
	}

	if (!key_params) {
		return false;
	}

	if (special_param_func) {
		if (special_param_func(old_settings, new_settings)) {
			blog(LOG_INFO, "[pls_check_key_params_changed] Special param changed");
			return true;
		}
	}

	for (size_t i = 0; i < key_params_count; ++i) {
		const struct pls_key_param_config *param = &key_params[i];
		if (!param || !param->key) {
			continue;
		}
		bool changed = false;

		switch (param->type) {
		case OBS_DATA_STRING: {
			const char *oldVal = obs_data_get_string(old_settings, param->key);
			const char *newVal = obs_data_get_string(new_settings, param->key);
			changed = (strcmp(oldVal ? oldVal : "", newVal ? newVal : "") != 0);
			if (changed) {
				blog(LOG_INFO,
				     "[pls_check_key_params_changed] Key param '%s' changed: old='%s', new='%s'",
				     param->key, oldVal ? oldVal : "", newVal ? newVal : "");
			}
			break;
		}
		case OBS_DATA_NUMBER: {
			long long oldVal = obs_data_get_int(old_settings, param->key);
			long long newVal = obs_data_get_int(new_settings, param->key);
			changed = (oldVal != newVal);
			if (changed) {
				blog(LOG_INFO,
				     "[pls_check_key_params_changed] Key param '%s' changed: old=%lld, new=%lld",
				     param->key, oldVal, newVal);
			}
			break;
		}
		case OBS_DATA_BOOLEAN: {
			bool oldVal = obs_data_get_bool(old_settings, param->key);
			bool newVal = obs_data_get_bool(new_settings, param->key);
			changed = (oldVal != newVal);
			if (changed) {
				blog(LOG_INFO, "[pls_check_key_params_changed] Key param '%s' changed: old=%d, new=%d",
				     param->key, oldVal, newVal);
			}
			break;
		}
		default:
			break;
		}

		if (changed) {
			return true;
		}
	}

	return false;
}
//PRISM/FanZirong/20251125/PRISM_PC-4540/avoid reactivating DShow device for non-key param changes -end

void *pls_get_source_action_helper(obs_source_t *source)
{
	if (!source)
		return NULL;

#ifdef PLS_UI_ACTION_STATS
	return source->action_helper_ptr;
#else
	return NULL;
#endif
}

void *pls_get_item_action_helper(obs_sceneitem_t *item)
{
	if (!item)
		return NULL;

#ifdef PLS_UI_ACTION_STATS
	return item->action_helper_ptr;
#else
	return NULL;
#endif
}

void *pls_get_scene_action_helper(obs_scene_t *item)
{
	if (!item)
		return NULL;

#ifdef PLS_UI_ACTION_STATS
	return item->action_helper_ptr;
#else
	return NULL;
#endif
}

#ifdef PLS_UI_ACTION_STATS
obs_source_t *pls_get_action_parent(obs_source_t *source)
{
	if (!source)
		return NULL;

	return source->action_parent;
}

void pls_set_action_parent(obs_source_t *source, obs_source_t *parent)
{
	if (!source)
		return;

	source->action_parent = parent;
}
#endif
