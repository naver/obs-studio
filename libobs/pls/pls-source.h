/*
* pls-source.h
* PRISM specificial api
* WuLongyue
* 2023-01-16
*/

#pragma once

#include "obs.h"

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
	void (*properties_edit_end)(void *source, obs_data_t *settings,
				    bool is_save_click);

	//PRISM/Zhangdewen/20230202/#/update extern params
	void (*update_extern_params)(void *data,
				     const calldata_t *extern_params);
	//PRISM/Zhangdewen/20230202/#/move from //PRISM/RenJinbo/20210603/#none/timer source feature
	void (*cef_dispatch_js)(void *data, const char *event_name,
				const char *json_data);
};

EXPORT void register_pls_source_info(struct obs_source_info *obs_info,
				     struct pls_source_info *pls_info);

EXPORT bool pls_source_set_private_data(obs_source_t *source, obs_data_t *data);
EXPORT bool pls_source_get_private_data(obs_source_t *source, obs_data_t *data);

EXPORT bool pls_plugin_set_private_data(const char *id, obs_data_t *data);
EXPORT bool pls_plugin_get_private_data(const char *id, obs_data_t *out_data);

EXPORT bool pls_source_properties_edit_start(obs_source_t *source);
EXPORT bool pls_source_properties_edit_end(obs_source_t *source,
					   bool is_save_click);

//PRISM/Zhangdewen/20230202/#/update extern params
EXPORT bool pls_source_update_extern_params(obs_source_t *source,
					    const calldata_t *extern_params);
//PRISM/Zhangdewen/20230202/#/update extern params, calldata_t: { cjson:const char*, sub_code:int }
EXPORT bool pls_source_update_extern_params_json(obs_source_t *source,
						 const char *cjson,
						 int sub_code);
//PRISM/Zhangdewen/20230202/#/move from //PRISM/RenJinbo/20210603/#none/timer source feature
EXPORT bool pls_source_dispatch_cef_js(const obs_source_t *source,
				       const char *event_name,
				       const char *json_data);
EXPORT void pls_source_invoke_method(obs_source_t *source, const char *method);

EXPORT void pls_init_alive_module();
EXPORT void pls_add_alive(void *s);
EXPORT void pls_remove_alive(void *s);
EXPORT bool pls_is_alive(void *s);

#ifdef __cplusplus
}
#endif
