#include "pls-source.h"
#include "obs-internal.h"

static void free_pls_data(void *type_data)
{
	struct pls_source_info *pls_source_info =
		(struct pls_source_info *)type_data;

	bfree(pls_source_info);
}

const int IS_PLS_SOURCE = 0x63278228;

void register_pls_source_info(struct obs_source_info *obs_info,
			      struct pls_source_info *pls_info)
{
	pls_info->is_pls_source = IS_PLS_SOURCE;
	obs_info->type_data = bmemdup(pls_info, sizeof(struct pls_source_info));
	obs_info->free_type_data = free_pls_data;
}

#define pls_source_invoke_func(FuncName, ...)                             \
	if (source->context.data && source->info.type_data) {             \
		struct pls_source_info *pls_source_info =                 \
			(struct pls_source_info *)source->info.type_data; \
		if (IS_PLS_SOURCE != pls_source_info->is_pls_source) {    \
			return false;                                     \
		}                                                         \
		if (pls_source_info->FuncName) {                          \
			pls_source_info->FuncName(source->context.data,   \
						  ##__VA_ARGS__);         \
			return true;                                      \
		}                                                         \
	}                                                                 \
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

	const struct pls_source_info *pls_source_info =
		(struct pls_source_info *)info->type_data;
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

	const struct pls_source_info *pls_source_info =
		(struct pls_source_info *)info->type_data;
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
	pls_source_invoke_func(properties_edit_end, source->context.settings,
			       is_save_click);
}

//PRISM/Zhangdewen/20230202/#/update extern params
bool pls_source_update_extern_params(obs_source_t *source,
				     const calldata_t *extern_params)
{
	pls_source_invoke_func(update_extern_params, extern_params);
}
//PRISM/Zhangdewen/20230202/#/update extern params, calldata_t: { cjson:const char*, sub_code:int }
bool pls_source_update_extern_params_json(obs_source_t *source,
					  const char *cjson, int sub_code)
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
bool pls_source_dispatch_cef_js(const obs_source_t *source,
				const char *event_name, const char *json_data)
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

//---------------------------------------- alive module start ----------------------------------------
pthread_mutex_t mutex_alive;
DARRAY(void *) alive_list;

void pls_init_alive_module()
{
	da_init(alive_list);
	pthread_mutex_init_value(&mutex_alive);
	pthread_mutex_init(&mutex_alive, NULL);
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

//---------------------------------------- alive module end ----------------------------------------
