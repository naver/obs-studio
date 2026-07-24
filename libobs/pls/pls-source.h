/*
* pls-source.h
* PRISM specificial api
* WuLongyue
* 2023-01-16
*/

#pragma once

#include "obs.h"
#include "pls/pls-action-util.h"

#ifdef __cplusplus
extern "C" {
#endif

//PRISM/Zhongling/20230809/#/lens and mobile plugin
#define TEXT_PRISM_LENS_NAME "main.prism.lens.name"
#define TEXT_PRISM_LENS_MOBILE_NAME "main.prism.lens.mobile.name"

#define TEXT_PRISM_LENS_TIPS "main.property.lens.tips"
#define TEXT_PRISM_LENS_MOBILE_TIPS "main.property.mobile.tips"

#define TEXT_PRISM_LENS_DEVICE "main.property.prism.lens.device"

#define TEXT_PRISM_LENS_AUDIO_DEVICE "main.property.prism.lens.audio"

#define TEXT_PRISM_LENS_AUDIO_TIPS_KEY "main.property.prism.lens.audio"
#define TEXT_PRISM_LENS_AUDIO_TIPS "main.property.prism.lens.mac.audio.desc"

#define TEXT_PRISM_LENS_OPEN_KEY "openPrismLens"
#define TEXT_PRISM_LENS_OPEN_TEXT "main.property.prism.lens.open"

#define TEXT_PRISM_LENS_ID "prism_lens"
#define TEXT_PRISM_LENS_MOBILE_ID "prism_lens_mobile"

#define TEXT_PRISM_LENS_1 "PRISM Lens 1"
#define TEXT_PRISM_LENS_2 "PRISM Lens 2"
#define TEXT_PRISM_LENS_3 "PRISM Lens 3"

#define TEXT_PRISM_LENS_AUDIO_1 "PRISM Lens audio 1"
#define TEXT_PRISM_LENS_AUDIO_2 "PRISM Lens audio 2"
#define TEXT_PRISM_LENS_AUDIO_3 "PRISM Lens audio 3"
#define METHOD_REFRESH_BROWSER "RefreshBrowser"
#define METHOD_SHOW_INTERACTION "ShowInteract"
#define METHOD_HIDE_INTERACTION "HideInterct"
//PRISM/Zhongling/20230809/#/lens and mobile plugin

enum pls_icon_type {
	PLS_ICON_TYPE_BASE = 0x38af0000,

	PLS_ICON_TYPE_BGM,
	PLS_ICON_TYPE_NDI,
	PLS_ICON_TYPE_TEXT_TEMPLATE,
	PLS_ICON_TYPE_CHAT,
	PLS_ICON_TYPE_REGION,
	PLS_ICON_TYPE_GIPHY,
	PLS_ICON_TYPE_SPECTRALIZER,
	PLS_ICON_TYPE_VIRTUAL_BACKGROUND,
	PLS_ICON_TYPE_PRISM_MOBILE,
	PLS_ICON_TYPE_PRISM_STICKER,
	PLS_ICON_TYPE_PRISM_TIMER,
	PLS_ICON_TYPE_OBS_CAMERA,
	PLS_ICON_TYPE_APP_AUDIO,
	PLS_ICON_TYPE_VIEWER_COUNT,
	PLS_ICON_TYPE_DECKLINK_INPUT,

	//PRISM/Liuying/20230808/new ux : add prism lens source
	PLS_ICON_TYPE_PRISM_LENS,
	PLS_ICON_TYPE_SPOUT2,
	PLS_ICON_TYPE_CHAT_TEMPLATE,
	PLS_ICON_TYPE_CHZZK_SPONSOR,
	PLS_ICON_TYPE_CAPTURE_CARD
};

struct pls_source_info {
	int is_pls_source;
	void (*set_private_data)(void *source, obs_data_t *data);
	void (*get_private_data)(void *source, obs_data_t *data);

	void (*properties_edit_start)(void *source, obs_data_t *settings);
	void (*properties_edit_end)(void *source, obs_data_t *settings, bool is_save_click);

	//PRISM/Zhangdewen/20230202/#/update extern params
	void (*update_extern_params)(void *data, const calldata_t *extern_params);
	//PRISM/Zhangdewen/20230202/#/move from //PRISM/RenJinbo/20210603/#none/timer source feature
	void (*cef_dispatch_js)(void *data, const char *event_name, const char *json_data);

	void (*check_obs_source_settings)(void *source, obs_data_t *output);

	//PRISM/chenguoxi/20250422/PRISM_PC-2756/delete occupied resources
	void (*free_resources)(void *source);
};

EXPORT void register_pls_source_info(struct obs_source_info *obs_info, struct pls_source_info *pls_info);

EXPORT bool pls_source_set_private_data(obs_source_t *source, obs_data_t *data);
EXPORT bool pls_source_get_private_data(obs_source_t *source, obs_data_t *data);

EXPORT bool pls_plugin_set_private_data(const char *id, obs_data_t *data);
EXPORT bool pls_plugin_get_private_data(const char *id, obs_data_t *out_data);

EXPORT bool pls_source_properties_edit_start(obs_source_t *source);
EXPORT bool pls_source_properties_edit_end(obs_source_t *source, bool is_save_click);

//PRISM/Zhangdewen/20230202/#/update extern params
EXPORT bool pls_source_update_extern_params(obs_source_t *source, const calldata_t *extern_params);
//PRISM/Zhangdewen/20230202/#/update extern params, calldata_t: { cjson:const char*, sub_code:int }
EXPORT bool pls_source_update_extern_params_json(obs_source_t *source, const char *cjson, int sub_code);
//PRISM/Zhangdewen/20230202/#/move from //PRISM/RenJinbo/20210603/#none/timer source feature
EXPORT bool pls_source_dispatch_cef_js(const obs_source_t *source, const char *event_name, const char *json_data);
EXPORT void pls_source_invoke_method(obs_source_t *source, const char *method);

//PRISM/chenguoxi/20241216/PRISM_PC-1778/audio has cover
typedef enum {
	MP_NOT_READY = 0,
	MP_NO_COVER,
	MP_HAS_COVER,
} AUDIO_COVER_STATUS;
EXPORT AUDIO_COVER_STATUS pls_source_is_audio_has_cover(obs_source_t *source);

//PRISM/chenguoxi/20250422/PRISM_PC-2756/delete occupied resources
EXPORT bool pls_source_free_resources(obs_source_t *source);

EXPORT void pls_init_alive_module();
EXPORT void pls_add_alive(void *s);
EXPORT void pls_remove_alive(void *s);
EXPORT bool pls_is_alive(void *s);

EXPORT void pls_add_sampler_alive(void *s);
EXPORT void pls_remove_sampler_alive(void *s);
EXPORT bool pls_sampler_is_alive(void *s);

EXPORT void pls_update_source_loading(obs_source_t *source, bool loading);
EXPORT bool pls_is_source_loading(obs_source_t *source);
EXPORT void pls_check_end_loading(obs_source_t *source);

//PRISM/wangshaohui/20250408/PRISM_PC-1636/add source api for paid
// this is for keyname in API, we should confuse its name.
#define API_PAID_KEY_NAME "obs_text_font_attritube_flag"
EXPORT bool obs_source_check_settings_ex(obs_source_t *source, obs_data_t *output);

//PRISM/chenguoxi/20250422/PRISM_PC-2756/delete occupied resources
EXPORT bool pls_stop_ffmpeg_source(obs_source_t *source);

//PRISM/lizhiyong/20251013/PRISM_PC-4169/crash when create source failed
EXPORT bool pls_source_context_data_valid(obs_source_t *source);

//PRISM/FanZirong/20251125/PRISM_PC-4540/avoid reactivating DShow device for non-key param changes -start
struct pls_key_param_config {
	const char *key;
	enum obs_data_type type;
};

EXPORT bool pls_check_key_params_changed(obs_data_t *new_settings, obs_data_t *old_settings,
					 const struct pls_key_param_config *key_params, size_t key_params_count,
					 bool (*special_param_func)(obs_data_t *old_settings,
								    obs_data_t *new_settings));
//PRISM/FanZirong/20251125/PRISM_PC-4540/avoid reactivating DShow device for non-key param changes -end

#ifdef __cplusplus
}
#endif
