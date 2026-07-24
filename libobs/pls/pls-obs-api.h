#pragma once

#include "util/c99defs.h"
#include "obs.h"
#include "graphics/graphics.h"

#ifdef __cplusplus
extern "C" {
#endif
enum obs_source_event_type {
	OBS_SOURCE_EXCEPTION_NONE = 0,
	OBS_SOURCE_EXCEPTION_SENSETIME,
	OBS_SOURCE_EXCEPTION_D3D,
	//PRISM/Xiewei/20210429/#7810/To handle device stop pushing frame.
	OBS_SOURCE_EXCEPTION_VIDEO_DEVICE,
	OBS_SOURCE_EXCEPTION_NO_FILE,
	OBS_SOURCE_EXCEPTION_BG_FILE_ERROR,
	OBS_SOURCE_SENSEAR_ACTION,
	OBS_SOURCE_EXCEPTION_BG_FILE_NETWORK_ERROR,
	OBS_SOURCE_VIRTUAL_BACKGROUND_STATUS,
	//PRISM/Liuying/20200126/add event
	OBS_SOURCE_CREATED_FINISHED,
	OBS_SOURCE_DEVICE_UNSTABLE,
	OBS_SOURCE_DEVICE_ACTION,
	OBS_SOURCE_VST_CHANGED,
	//PRISM/Zhangdewen/20211015/#/Chat Source Event
	OBS_SOURCE_CHAT_UPDATE_PARAMS,
	//PRISM/Xiewei/20220321/None/for capturing camera image result
	OBS_SOURCE_CAMERA_CAPTURE_IMAGE,
	//PRISM/WangChuanjing/20220512/None/for beauty preset lost
	OBS_SOURCE_BEAUTY_PRESET_NOT_EXIST,
	//PRISM/WangChuanjing/20220517/None/for beauty lab
	OBS_SOURCE_BEAUTY_INPUT_MODEL_NOT_EXIST,
	OBS_SOURCE_BEAUTY_INPUT_MODEL_NO_FACE,
	OBS_SOURCE_BEAUTY_INPUT_MODEL_ANGLE_OUT_OF_RANGE,
	//PRISM/Zhangdewen/20220819/#/Viewer Count Source Event
	OBS_SOURCE_BROWSER_LOADED,
	OBS_SOURCE_VIEWER_COUNT_UPDATE_PARAMS,
	//PRISM/ChengBing/20221102/#textmotion pos and size change Event
	OBS_SOURCE_TEXTMOTION_POS_CHANGED,
	//PRISM/LiuYing/20230202/#music state changed
	OBS_SOURCE_MUSIC_STATE_CHANGED,

	//PRISM/LiuYing/20230224/#music loop state changed
	OBS_SOURCE_MUSIC_LOOP_STATE_CHANGED,
	//PRISM/LiuYing/20240326/#music mode state changed
	OBS_SOURCE_MUSIC_MODE_STATE_CHANGED,
	// game capture message
	PLS_SOURCE_GAME_CAPTURE_FAILED_MSG,
	//PRISM/Renjinbo/20230505/#timer button state changed
	OBS_SOURCE_TIMER_BUTTON_STATE_CHANGED,
	//PRISM/Chengbing/20230529/#/text template Source update param
	OBS_SOURCE_TEXT_TEMPLATE_UPDATE_PARAMS,
	// game capture success msg
	PLS_SOURCE_GAME_CAPTURE_SUCCESS_MSG,
	//PRISM/chenguoxi/20251103/PRISM_PC-3578/window and monitor capture failed guidance
	OBS_SOURCE_FAILED_STATUS,
};

//PRISM/FanZirong/20251103/PRISM_PC-3577/source capture failed guidance
enum obs_source_failed_status_sub_code {
	OBS_SOURCE_STATUS_SUCCESS = 0,
	// game capture failed
	OBS_SOURCE_GAME_CAPTURE_FAILED_SUB_CODE_OPEN_TARGET_PROCESS = 1,
	OBS_SOURCE_GAME_CAPTURE_FAILED_SUB_CODE_BLACKLISTED_PROCESS = 2,
	OBS_SOURCE_GAME_CAPTURE_FAILED_SUB_CODE_TARGET_SUSPENDED = 3,
	OBS_SOURCE_GAME_CAPTURE_FAILED_SUB_CODE_INIT_PIPE = 4,
	OBS_SOURCE_GAME_CAPTURE_FAILED_SUB_CODE_HOOK_DIRECT_FAIL = 5,
	OBS_SOURCE_GAME_CAPTURE_FAILED_SUB_CODE_HOOK_DIRECT_HELPER_FAIL = 6,
	OBS_SOURCE_GAME_CAPTURE_FAILED_SUB_CODE_INIT_CAPTURE_DATA_FAIL = 7,
	OBS_SOURCE_GAME_CAPTURE_FAILED_SUB_CODE_CREATE_TEXTURE_FAIL = 8,
	OBS_SOURCE_GAME_CAPTURE_FAILED_SUB_CODE_OPEN_SHARED_HANDLE_FAIL = 9,
	OBS_SOURCE_GAME_CAPTURE_FAILED_SUB_CODE_NO_INJECT_HELPER = 10,
	OBS_SOURCE_GAME_CAPTURE_FAILED_SUB_CODE_NO_INJECT_DLL = 11,
	OBS_SOURCE_GAME_CAPTURE_FAILED_SUB_CODE_INIT_KEEPALIVE = 12,

	// window capture failed
	OBS_SOURCE_WINDOW_CAPTURE_FAILED_SUB_CODE_WINDOW_INVALID = 201,
	OBS_SOURCE_WINDOW_CAPTURE_FAILED_SUB_CODE_WINDOW_INVISIBE = 202,
	OBS_SOURCE_WINDOW_CAPTURE_FAILED_SUB_CODE_UNKNOWN = 299,

	// monitor capture failed
	OBS_SOURCE_MONITOR_CAPTURE_FAILED_SUB_CODE_MONITOR_INVALID = 301,
	OBS_SOURCE_MONITOR_CAPTURE_FAILED_SUB_CODE_UNKNOWN = 399,

	// mac capture failed
	OBS_SOURCE_MAC_CAPTURE_FAILED_SUB_CODE_NO_PERMISSION = 401,
	OBS_SOURCE_MAC_CAPTURE_FAILED_SUB_CODE_NO_CONTENT = 402,
	OBS_SOURCE_MAC_CAPTURE_FAILED_SUB_CODE_UNKNOWN = 499,

	// win dshow capture failed
	OBS_SOURCE_DSHOW_CAPTURE_FAILED_SUB_DEVICE_REMOVED = 501,
	OBS_SOURCE_DSHOW_CAPTURE_FAILED_SUB_DEVICE_IN_USED = 502,
	OBS_SOURCE_DSHOW_CAPTURE_FAILED_SUB_DEVICE_NOT_FOUND = 503,
	OBS_SOURCE_DSHOW_CAPTURE_FAILED_SUB_DEVICE_NOT_SUPPORT_HDR = 504,
	OBS_SOURCE_DSHOW_CAPTURE_FAILED_SUB_DEVICE_NOT_SUPPORT_POPERTY = 505,
	OBS_SOURCE_DSHOW_CAPTURE_FAILED_SUB_UNKNOWN = 599,

	// mac av capture failed
	OBS_SOURCE_MAC_AVCAPTURE_FAILED_SUB_DEVICE_NO_PERMISSION = 601,
	OBS_SOURCE_MAC_AVCAPTURE_FAILED_SUB_DEVICE_REMOVED = 602,
	OBS_SOURCE_MAC_AVCAPTURE_FAILED_SUB_DEVICE_IN_USED = 603,
	OBS_SOURCE_MAC_AVCAPTURE_FAILED_SUB_DEVICE_NOT_FOUND = 604,
	OBS_SOURCE_MAC_AVCAPTURE_FAILED_SUB_DEVICE_NOT_SUPPORT_POPERTY = 605,
	OBS_SOURCE_MAC_AVCAPTURE_FAILED_SUB_UNKNOWN = 699,

	//win dhsow capture lens failed
	OBS_SOURCE_DSHOW_CAPTURE_LENS_FAILED_SUB_NOT_ACTIVE = 701,
	OBS_SOURCE_DSHOW_CAPTURE_LENS_FAILED_SUB_UNKNOWN = 799,

	//mac av capture lens failed
	OBS_SOURCE_MAC_AVCAPTURE_LENS_FAILED_SUB_NOT_ACTIVE = 801,
	OBS_SOURCE_MAC_AVCAPTURE_LENS_FAILED_SUB_UNKNOWN = 899,
};

//PRISM/Zhangdewen/20230203/#/Chat Source Event
enum obs_chat_update_params_notify_sub_code {
	OBS_SOURCE_CHAT_UPDATE_PARAMS_SUB_CODE_UPDATE,
	OBS_SOURCE_CHAT_UPDATE_PARAMS_SUB_CODE_EDIT_START,
	OBS_SOURCE_CHAT_UPDATE_PARAMS_SUB_CODE_LOADED,
	OBS_SOURCE_CHAT_UPDATE_PARAMS_SUB_CODE_CHECK_LIVE,
	OBS_SOURCE_CHAT_UPDATE_PARAMS_SUB_CODE_RESIZE_VIEW,
	OBS_SOURCE_CHAT_UPDATE_PARAMS_SUB_CODE_JSONLOADED
};
//PRISM/Chengbing/20230529/#/text template Source Event
enum obs_text_template_update_params_notify_sub_code {
	OBS_SOURCE_TEXT_TEMPLATE_UPDATE_PARAMS_SUB_CODE_UPDATE,
	OBS_SOURCE_TEXT_TEMPLATE_UPDATE_PARAMS_SUB_CODE_EDIT_START,
	OBS_SOURCE_TEXT_TEMPLATE_UPDATE_PARAMS_SUB_CODE_LOADED,
	OBS_SOURCE_TEXT_TEMPLATE_UPDATE_PARAMS_SUB_CODE_SIZECHANGED,
	OBS_SOURCE_TEXT_TEMPLATE_UPDATE_PARAMS_SUB_CODE_JSONLOADED
};
//PRISM/Zhangdewen/20230203/#/Viewer Count Source Event
enum obs_viewer_count_update_params_notify_sub_code {
	OBS_SOURCE_VIEWER_COUNT_UPDATE_PARAMS_SUB_CODE_UPDATE_URL,
	OBS_SOURCE_VIEWER_COUNT_UPDATE_PARAMS_SUB_CODE_UPDATE_PARAMS,
};

enum obs_vst_verify_state {
	VST_STATUS_AVAILABLE = 100,
	VST_STATUS_INVALID_ARCH,
	VST_STATUS_EFFECT_UNSUPPORT,
	VST_STATUS_TIMEOUT,
	VST_STATUS_CRASH,
	VST_STATUS_NOT_VST,
	VST_STATUS_CHANNEL_UNSUPPORT,
	VST_STATUS_DLL_LOAD_FAIL,
	VST_STATUS_UNKNOWN_ERROR,
	VST_STATUS_EFFECT_NULLPTR,

	//-----------------------------------------
	VST_STATUS_SCAN_RESULT_END,
	VST_STATUS_CHECKING,
	VST_STATUS_PROCESS_DISAPPEAR,
	VST_STATUS_PROCESS_READ_ERROR,
	VST_STATUS_PROCESS_WRITE_ERROR,
	VST_STATUS_PROCESS_FAILED_TO_START,
	VST_STATUS_PROCESS_OPEN_DLL_ERROR,
	VST_STATUS_PROCESS_GET_SCAN_FUNC_ERROR,
	VST_STATUS_PROCESS_UNKNOWN_ERROR,

	/* ON macos, exit code should between 0-255, values more than 255 will be moded by 256 */
	VST_ENUM_MAX_VALUE = 256,
};

//Limit min resolution to 4
#define RESOLUTION_SIZE_MIN 4
//Refer to OBS, limit max resolution to 16384
#define RESOLUTION_SIZE_MAX 16384
//PRISM/Xiewei/20220524/none/for laboratory plugin
#define OBS_SOURCE_FLAG_LABORATORY (1 << 2)

/*obs source apis*/
EXPORT void pls_vst_state_changed(const obs_source_t *source, const char *vst, enum obs_vst_verify_state state);
EXPORT void pls_set_wgc_borderless_enable(bool enable);
EXPORT bool pls_get_wgc_borderless_enable();
EXPORT void pls_source_send_notify(const obs_source_t *source, enum obs_source_event_type type, int sub_code);
EXPORT void pls_source_send_distinct_notify(const obs_source_t *source, enum obs_source_event_type type, int sub_code);
EXPORT void pls_source_send_message(const obs_source_t *source, enum obs_source_event_type type, obs_data_t *data);
EXPORT void pls_source_property_update_notify(const obs_source_t *source, const char *name);
EXPORT void pls_source_property_update_notify(const obs_source_t *source, const char *name);
EXPORT void pls_source_cef_received_web_msg(const obs_source_t *source, const char *msg);
EXPORT void pls_audio_output_get_info(uint32_t *samples_per_sec, int *speakers);
//PRISM/FanZirong/20250819/PRISM_PC-3614/add flip horizontally
EXPORT void obs_source_set_flip_horizontal(obs_source_t *source, bool flip_h);

//PRISM/FanZirong/20251112/PRISM_PC-3577/source capture failed guidance
EXPORT void pls_source_set_failed_status_sub_code(obs_source_t *source, enum obs_source_failed_status_sub_code code);
EXPORT enum obs_source_failed_status_sub_code pls_source_get_failed_status_sub_code(obs_source_t *source);
/*
* DO NOT forget to free the memory
*/
EXPORT char *pls_get_module_file_name_ptr(const char *module_name);
//PRISM/Zhangdewen/20230117/#/load with filter
typedef bool (*pls_load_module_filter_t)(const char *bin_path);
typedef void (*pls_load_module_load_callback_t)(void *param, const struct obs_module_info2 *info);
typedef void (*pls_load_module_load_t)(pls_load_module_load_callback_t callback, void *param,
				       const struct obs_module_info2 *info);
EXPORT void pls_load_all_modules(pls_load_module_filter_t filter, pls_load_module_load_t load);
//PRISM/Zhangdewen/20230117/#/load with filter
EXPORT void pls_load_all_modules2(struct obs_module_failure_info *mfi, pls_load_module_filter_t filter,
				  pls_load_module_load_t load);

//PRISM/Liuying/20230131/#/add load sources callback
typedef bool (*obs_load_pld_cb)(void *private_data, obs_source_t *source);
EXPORT void pls_load_sources(obs_data_array_t *array, obs_load_source_cb cb, obs_load_pld_cb pldCb, void *private_data,
			     void *pld_private_data);

//PRISM/Wangshaohui/20220422/#none/load plugin
/*
Note: It must be called from UI thread.
bin_path: "prism-plugins/afreecatv-login.dll"
data_path: "data/prism-plugins/afreecatv-login"
*/
EXPORT bool pls_load_plugin(const char *bin_path, const char *data_path);

//PRISM/Zengqin/20220311/#none/for DrawPen feature
EXPORT void pls_scene_update_canvas(obs_scene_t *scene, gs_texture_t *texture, bool save);
//PRISM/Zengqin/20220422/#none/for DrawPen feature
EXPORT gs_texture_t *pls_scene_get_canvas(obs_scene_t *scene);
//PRISM/Zengqin/20230201/#none/for DrawPen feature
EXPORT void pls_scene_canvas_render(void *data);
//PRISM/Zengqin/20230201/#none/for DrawPen feature
EXPORT void pls_duplicate_scene_canvas(obs_scene_t *dst, obs_scene_t *src);
//PRISM/Xiewei/20230210/#none/for Region capture
EXPORT uint64_t pls_texture_get_max_size();

//PRISM/RenJinbo/20200623/#None/add properties view ok button enable.
EXPORT void pls_source_properties_view_ok_button_enable(obs_source_t *source, bool enable);
typedef void (*log_callback)(const char *module_name, bool internal_module);
EXPORT void pls_log_loaded_modules(log_callback callback);
//PRISM/Zhongling/20230602/#none/for Draw pne
EXPORT gs_effect_t *pls_get_prism_effect();

//PRISM/WuLongyue/20230727/None/codec analog
EXPORT void pls_analog_codec_notify(const char *codec, const char *encodeDecode, bool hw);

//PRISM/Xiewei/20230712/#1881 filter repeated obs plugins start
EXPORT bool pls_is_dll_already_loaded(const char *dllName);
EXPORT void pls_remember_dll_name(const char *dllName);

#ifdef _WIN32
EXPORT void pls_start_recording_third_party_plugin();
EXPORT void pls_record_third_party_plugin(const char *dllName);
EXPORT void pls_finish_recording_third_party_plugin();
EXPORT bool os_is_pls_plugin(const char *path);
#endif

EXPORT bool pls_is_plugin_in_black_list(const char *dllName);

//PRISM/Zhongling/20231027/#2902/exit crashed
EXPORT void pls_set_obs_exiting(bool exiting);
EXPORT bool pls_get_obs_exiting();

//PRISM/AiGuanghua/20240624/#5561/source signal shut down crashed
EXPORT void pls_set_obs_shutdowning(bool shutdowning);
EXPORT bool pls_get_obs_shutdowning();

//PRISM/Chengbing/20231108/#/prism version
EXPORT void pls_update_prism_version(int major, int minor, int patch, int build);
EXPORT int pls_prism_version_major();
EXPORT int pls_prism_version_minor();
EXPORT int pls_prism_version_patch();
EXPORT int pls_prism_version_build();

#if defined(_WIN32)
EXPORT bool pls_get_enum_timeout_device(wchar_t *buffer, size_t size);
EXPORT void pls_extract_file_name(const char *full_path, char *out_buf, int buf_len);
// must be called after GDI+ startup
EXPORT void pls_init_font_collection();
// must be called before GDI+ shutdown
EXPORT void pls_destory_font_collection();
EXPORT void pls_enter_font_collection();
EXPORT void pls_leave_font_collection();
EXPORT void *pls_get_private_font_collection();
EXPORT bool pls_add_font_to_private_collection(const char *font_path);
#endif

EXPORT signal_handler_t *pls_freetype_get_signal();
EXPORT void pls_freetype_add_font(const char *font_path);
EXPORT const char *pls_freetype_pop_font_path();
EXPORT bool pls_freetype_needs_reload();
EXPORT void pls_freetype_set_needs_reload(bool reload);

EXPORT void pls_set_all_mute(bool mute);
EXPORT bool pls_get_all_mute();

EXPORT size_t pls_get_active_output_count();
//DON'T save result of this function
EXPORT const char *pls_get_active_output_name(size_t);
EXPORT void pls_set_design_mode(bool enable);
EXPORT bool pls_design_mode();

//PRISM/fanzirong/20240704/none/separate stop and free
EXPORT void stop_audio_thread(audio_t *audio);

/** Create dual output scene **/
EXPORT obs_scene_t *pls_create_vertical_scene(const char *name, bool is_group_ref);

/** Set group-ref flag on existing vertical scene */
EXPORT void pls_vertical_scene_set_group_ref(obs_scene_t *scene, bool is_group_ref);

/** Enum scenes list (except scenes for dual output)**/
EXPORT void pls_enum_all_scenes(bool (*enum_proc)(void *, obs_source_t *), void *param);

EXPORT void pls_set_dev_mode(bool dev);
EXPORT bool pls_is_dev_mode();

EXPORT void pls_set_local_log(bool local_log);
EXPORT bool pls_is_local_log();

EXPORT void pls_on_game_render_type(const char *game_title, const char *render_type);

EXPORT void pls_add_id_module_map(const char *plugin_id, obs_module_t *plugin_module);
EXPORT void pls_remove_id_module_map_by_key(const char *plugin_id);
EXPORT void pls_remove_id_module_map_by_value(obs_module_t *plugin_module);
EXPORT void pls_add_module_lookup_map(obs_module_t *plugin_module, lookup_t *lookup);
EXPORT void pls_remove_module_lookup_map_by_key(obs_module_t *plugin_module);
EXPORT void pls_remove_module_lookup_map_by_value(lookup_t *lookup);
EXPORT lookup_t *pls_get_lookup_by_id(const char *plugin_id);
EXPORT void pls_add_getstring_pointer_module_map(void *getstring_pointer, obs_module_t *plugin_module);
EXPORT void pls_remove_getstring_pointer_module_map_by_key(void *getstring_pointer);
EXPORT void pls_remove_getstring_pointer_module_map_by_value(obs_module_t *plugin_module);
EXPORT lookup_t *pls_get_lookup_by_getstring_pointer(void *getstring_pointer);
EXPORT void pls_add_text_lookup(lookup_t *lookup);
EXPORT void pls_remove_text_lookup(lookup_t *lookup);
EXPORT bool pls_is_valid_text_lookup(lookup_t *lookup);
EXPORT const char *obs_output_get_last_english_error(obs_output_t *output);

EXPORT void pls_update_pc_sleep(bool is_sleep);
EXPORT bool pls_ignore_render_drop();

EXPORT void pls_source_clear_async_video(obs_source_t *source);

#ifdef __cplusplus
}
#endif
