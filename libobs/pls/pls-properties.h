/*
* pls-properties.h
* PRISM specificial properties
* WuLongyue
* 2022-12-26
*/

#pragma once

#include "obs-properties.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*pls_property_checkbox_clicked_t)(obs_properties_t *props,
						obs_property_t *p, bool checked,
						void *context);

enum pls_property_type {
	//A random magic number, for distinguishing obs&prism property
	OBS_PROPERTY_BASE = 0x6af80000,

	PLS_PROPERTY_TIPS, //OBS_PROPERTY_MOBILE_GUIDER
	PLS_PROPERTY_LINE, //OBS_PROPERTY_H_LINE
	PLS_PROPERTY_TEXT_CONTENT,

	PLS_PROPERTY_BOOL_GROUP,   //OBS_PROPERTY_BOOL_GROUP
	PLS_PROPERTY_BUTTON_GROUP, //OBS_PROPERTY_BUTTON_GROUP
	PLS_PROPERTY_CUSTOM_GROUP,

	PLS_PROPERTY_BGM_LIST, //OBS_PROPERTY_BGM_MUSIC_LIST
	PLS_PROPERTY_BGM_TIPS, //OBS_PROPERTY_TIPS

	PLS_PROPERTY_CHAT_TEMPLATE_LIST, //OBS_PROPERTY_CHAT_TEMPLATE_LIST
	PLS_PROPERTY_CHAT_FONT_SIZE,     //OBS_PROPERTY_CHAT_FONT_SIZE

	PLS_PROPERTY_TM_TAB,           //OBS_PROPERTY_TM_TAB
	PLS_PROPERTY_TM_TEMPLATE_TAB,  //OBS_PROPERTY_TM_TEMPLATE_TAB
	PLS_PROPERTY_TM_TEMPLATE_LIST, //OBS_PROPERTY_TM_TEMPLATE_LIST
	PLS_PROPERTY_TM_DEFAULT_TEXT,  //OBS_PROPERTY_TM_DEFAULT_TEXT
	PLS_PROPERTY_TM_TEXT_CONTENT,  //OBS_PROPERTY_TM_TEXT_CONTENT
	PLS_PROPERTY_TM_TEXT,          //OBS_PROPERTY_TM_TEXT
	PLS_PROPERTY_TM_COLOR,         //OBS_PROPERTY_TM_COLOR
	PLS_PROPERTY_TM_MOTION,        //OBS_PROPERTY_TM_MOTION

	PLS_PROPERTY_REGION_SELECT, //OBS_PROPERTY_REGION_SELECT

	PLS_PROPERTY_IMAGE_GROUP,             //OBS_PROPERTY_IMAGE_GROUP
	PLS_PROPERTY_VISUALIZER_CUSTOM_GROUP, //OBS_PROPERTY_CUSTOM_GROUP

	PLS_PROPERTY_BOOL_LEFT, //OBS_PROPERTY_BOOL_LEFT

	PLS_PROPERTY_VIRTUAL_BACKGROUND_CAMERA_STATE, //OBS_PROPERTY_CAMERA_VIRTUAL_BACKGROUND_STATE
	PLS_PROPERTY_VIRTUAL_BACKGROUND_RESOURCE, //OBS_PROPERTY_VIRTUAL_BACKGROUND_RESOURCE
	PLS_PROPERTY_VIRTUAL_BACKGROUND_SWITCH, //OBS_PROPERTY_SWITCH,

	PLS_PROPERTY_MOBILE_HELP,  //OBS_PROPERTY_MOBILE_HELP,
	PLS_PROPERTY_MOBILE_NAME,  //OBS_PROPERTY_MOBILE_NAME
	PLS_PROPERTY_MOBILE_STATE, //OBS_PROPERTY_MOBILE_STATUS

	PLS_PROPERTY_FONT_SIMPLE,    //OBS_PROPERTY_FONT_SIMPLE
	PLS_PROPERTY_COLOR_CHECKBOX, //OBS_PROPERTY_COLOR_CHECKBOX

	PLS_PROPERTY_AUDIO_METER, //OBS_PROPERTY_AUDIO_METER

	PLS_PROPERTY_TEMPLATE_LIST,        //OBS_PROPERTY_TEMPLATE_LIST
	PLS_PROPERTY_COLOR_ALPHA_CHECKBOX, //OBS_PROPERTY_COLOR_ALPHA_CHECKBOX

	PLS_PROPERTY_CT_DISPLAY,//chat source display
	PLS_PROPERTY_CT_OPTIONS,//chat source options
	PLS_PROPERTY_CT_MOTION,//chat source motion
	PLS_PROPERTY_CT_FONT,//chat source font
	PLS_PROPERTY_CT_TEXT_COLOR,//chat source text color
	PLS_PROPERTY_CT_BK_COLOR,//chat source bk color
	PLS_PROPERTY_CHZZK_SPONSOR, // chzzk sponsor

};

//PRISM/Wangshaohui/20200927/#4868/for property UI
#define PROPERTY_FLAG_NO_LABEL_HEADER 1 << 0
//PRISM/Zengqin/20201125/no label for single property.
#define PROPERTY_FLAG_NO_LABEL_SINGLE 1 << 1
//PRISM/Zengqin/20201125/child control for property.
#define PROPERTY_FLAG_CHILD_CONTROL 1 << 2
//PRISM/Liuying/20201202/default audio device.
#define DEFAULT_AUDIO_DEVICE_FLAG 1 << 3
//PRSIM/RenJinbo/20210615/add fixed button width.
#define PROPERTY_FLAG_BUTTON_WIDTH_FIXED 1 << 4
//PRISM/Wangshaohui/20220420/none/for property UI
#define PROPERTY_FLAG_NO_TEXT_CUT 1 << 5
#define PROPERTY_FLAG_LIST_TIMER_LISTEN 1 << 6

enum pls_image_style_type {
	PLS_IMAGE_STYLE_UNKNOWN = -1,
	PLS_IMAGE_STYLE_TEMPLATE,
	PLS_IMAGE_STYLE_SOLID_COLOR,
	PLS_IMAGE_STYLE_GRADIENT_COLOR,
	PLS_IMAGE_STYLE_BORDER_BUTTON,
	PLS_IMAGE_STYLE_APNG_BUTTON,
};

enum pls_custom_group_type {
	PLS_CUSTOM_GROUP_UNKNOWN,
	PLS_CUSTOM_GROUP_BUTTON,
	PLS_CUSTOM_GROUP_BOOL,
	PLS_CUSTOM_GROUP_CHECKBOX,
	PLS_CUSTOM_GROUP_INT,
	PLS_CUSTOM_GROUP_FLOAT,
};
#pragma region common
EXPORT obs_properties_t *pls_properties_create(void);

EXPORT void pls_properties_destroy(obs_properties_t *props);

//obs_properties_set_flags
EXPORT void pls_property_set_flags(obs_property_t *p, uint32_t flags);
//obs_properties_get_flags
EXPORT uint32_t pls_property_get_flags(obs_property_t *p);

//obs_property_set_placeholder
EXPORT void pls_property_set_placeholder(obs_property_t *p,
					 const char *placeholder);
//obs_property_placeholder
EXPORT const char *pls_property_get_placeholder(obs_property_t *p);

//obs_property_set_length_limit
EXPORT void pls_property_set_length_limit(obs_property_t *p, int max_length);

//obs_property_get_length_limit
EXPORT int pls_property_get_length_limit(obs_property_t *p);

//obs_properties_add_mobile_guider
EXPORT obs_property_t *pls_properties_add_tips(obs_properties_t *props,
					       const char *name,
					       const char *desc);
EXPORT obs_property_t *pls_properties_add_tips_ex(obs_properties_t *props,
						  const char *name,
						  const char *desc,
						  bool whole_row);

EXPORT bool pls_property_tips_whole_row(obs_property_t *p);

//obs_properties_add_h_line
EXPORT obs_property_t *pls_properties_add_line(obs_properties_t *props,
					       const char *name,
					       const char *desc);
EXPORT obs_property_t *pls_properties_add_line_ex(obs_properties_t *props,
						  const char *name,
						  bool whole_row);

EXPORT bool pls_property_line_whole_row(obs_property_t *p);

EXPORT obs_property_t *pls_properties_add_text_content(obs_properties_t *props,
						       const char *name,
						       const char *desc,
						       const char *content);

EXPORT const char *pls_property_get_text_content(obs_property_t *p);
#pragma endregion

#pragma region bool_group
//obs_properties_add_bool_group
EXPORT obs_property_t *pls_properties_add_bool_group(obs_properties_t *props,
						     const char *name,
						     const char *desc);

EXPORT void pls_property_bool_group_clear(obs_property_t *p);

//obs_property_bool_group_clicked
EXPORT bool pls_property_bool_group_clicked(obs_property_t *p, void *context,
					    size_t idx);

//obs_property_bool_group_item_count
EXPORT size_t pls_property_bool_group_item_count(obs_property_t *p);

//obs_properties_add_bool_group_item
EXPORT size_t pls_property_bool_group_add_item(obs_property_t *p,
					       const char *name,
					       const char *desc,
					       const char *tooltip,
					       obs_property_clicked_t callback);

EXPORT size_t pls_property_bool_group_add_item_ex(
	obs_property_t *p, const char *name, const char *desc,
	const char *tooltip, bool enabled, bool visible,
	obs_property_clicked_t callback);

EXPORT const char *pls_property_bool_group_item_name(obs_property_t *p,
						     size_t idx);

//obs_property_bool_group_item_text
EXPORT const char *pls_property_bool_group_item_desc(obs_property_t *p,
						     size_t idx);

//obs_property_bool_group_item_tooltip
EXPORT const char *pls_property_bool_group_item_tooltip(obs_property_t *p,
							size_t idx);
//obs_property_bool_group_item_enabled
EXPORT bool pls_property_bool_group_item_enabled(obs_property_t *p, size_t idx);

//obs_property_bool_group_set_item_enabled
EXPORT void pls_property_bool_group_set_item_enabled(obs_property_t *p,
						     size_t idx, bool enabled);

EXPORT bool pls_property_bool_group_item_visible(obs_property_t *p, size_t idx);
#pragma endregion

#pragma region button_group
//obs_properties_add_button_group
EXPORT obs_property_t *pls_properties_add_button_group(obs_properties_t *props,
						       const char *name,
						       const char *desc);

//obs_property_button_group_clear
EXPORT void pls_property_button_group_clear(obs_property_t *p);

//obs_property_button_group_clicked
EXPORT bool pls_property_button_group_clicked(obs_property_t *p, void *context,
					      size_t idx);

//obs_property_button_group_item_count
EXPORT size_t pls_property_button_group_item_count(obs_property_t *p);

//obs_property_button_group_add_item
EXPORT size_t pls_property_button_group_add_item(
	obs_property_t *p, const char *name, const char *desc, bool enabled,
	obs_property_clicked_t callback);

EXPORT size_t pls_property_button_group_add_item_ex(
	obs_property_t *p, const char *name, const char *desc,
	const char *tooltip, bool enabled, bool visible, bool highlight,
	obs_property_clicked_t callback);

//obs_property_button_group_item_name
EXPORT const char *pls_property_button_group_item_name(obs_property_t *p,
						       size_t idx);

//obs_property_button_group_item_text
EXPORT const char *pls_property_button_group_item_desc(obs_property_t *p,
						       size_t idx);

//obs_property_button_group_set_item_text
EXPORT void pls_property_button_group_set_item_desc(obs_property_t *p,
						    size_t idx,
						    const char *desc);

EXPORT const char *pls_property_button_group_item_tooltip(obs_property_t *p,
							  size_t idx);

//obs_property_button_group_item_enable
EXPORT bool pls_property_button_group_item_enabled(obs_property_t *p,
						   size_t idx);

//obs_property_button_group_set_item_enable
EXPORT void pls_property_button_group_set_item_enabled(obs_property_t *p,
						       size_t idx,
						       bool highlight);

EXPORT bool pls_property_button_group_item_visible(obs_property_t *p,
						   size_t idx);

EXPORT void pls_property_button_group_set_item_visible(obs_property_t *p,
						       size_t idx,
						       bool visible);

//obs_property_button_group_set_item_highlight
EXPORT void pls_property_button_group_set_item_highlight(obs_property_t *p,
							 size_t idx,
							 bool highlight);

//obs_property_button_group_get_item_highlight
EXPORT bool pls_property_button_group_item_highlight(obs_property_t *p,
						     size_t idx);
#pragma endregion

#pragma region int_group
//obs_properties_add_int_group
EXPORT obs_property_t *pls_properties_add_int_group(obs_properties_t *props,
						    const char *name,
						    const char *desc);

//obs_property_int_group_item_count
EXPORT size_t pls_property_int_group_item_count(obs_property_t *p);

//obs_properties_add_int_group_item
EXPORT size_t pls_property_int_group_add_item(obs_property_t *p,
					      const char *name,
					      const char *desc, int min,
					      int max, int step);
//obs_property_int_group_item_params
EXPORT bool pls_property_int_group_item_params(obs_property_t *prop,
					       char **name, char **desc,
					       int *min, int *max, int *step,
					       size_t idx);

//obs_property_set_ignore_callback_when_refresh
EXPORT void pls_property_set_ignore_callback_when_refresh(obs_property_t *p,
							  bool ignored);

//obs_property_ignore_callback_when_refresh
EXPORT bool pls_property_get_ignore_callback_when_refresh(obs_property_t *p);
#pragma endregion

#pragma region custom group
EXPORT obs_property_t *pls_properties_add_custom_group(obs_properties_t *props,
						       const char *name,
						       const char *desc);

EXPORT void pls_property_custom_group_clear(obs_property_t *p);

EXPORT bool pls_property_custom_group_button_clicked(obs_property_t *p,
						     size_t idx, void *context);

EXPORT bool pls_property_custom_group_bool_clicked(obs_property_t *p,
						   size_t idx, void *context);

EXPORT bool pls_property_custom_group_checkbox_clicked(obs_property_t *p,
						       size_t idx, bool checked,
						       void *context);

EXPORT size_t pls_property_custom_group_item_count(obs_property_t *p);

EXPORT size_t pls_property_custom_group_add_button(
	obs_property_t *p, const char *name, const char *desc,
	const char *tooltip, bool enabled, bool visible, bool highlight,
	obs_property_clicked_t callback);

EXPORT size_t pls_property_custom_group_add_bool(
	obs_property_t *p, const char *name, const char *desc,
	const char *tooltip, bool enabled, bool visible,
	obs_property_clicked_t callback);

EXPORT size_t pls_property_custom_group_add_checkbox(
	obs_property_t *p, const char *name, const char *desc,
	const char *tooltip, bool enabled, bool visible,
	pls_property_checkbox_clicked_t callback);

EXPORT size_t pls_property_custom_group_add_int(
	obs_property_t *p, const char *name, const char *desc,
	const char *tooltip, bool enabled, bool visible, int min, int max,
	int step, const char *suffix, enum obs_number_type type);

EXPORT size_t pls_property_custom_group_add_float(
	obs_property_t *p, const char *name, const char *desc,
	const char *tooltip, bool enabled, bool visible, double min, double max,
	double step, const char *suffix, enum obs_number_type type);

EXPORT enum pls_custom_group_type
pls_property_custom_group_item_type(obs_property_t *p, size_t idx);

EXPORT bool pls_property_custom_group_item_params(obs_property_t *p, size_t idx,
						  const char **name,
						  const char **desc,
						  const char **tooltip,
						  bool *enabled, bool *visible);

EXPORT bool pls_property_custom_group_item_button_params(obs_property_t *p,
							 size_t idx,
							 bool *highlight);

EXPORT bool pls_property_custom_group_item_int_params(
	obs_property_t *p, size_t idx, int *min, int *max, int *step,
	const char **suffix, enum obs_number_type *type);

EXPORT bool pls_property_custom_group_item_float_params(
	obs_property_t *p, size_t idx, double *min, double *max, double *step,
	const char **suffix, enum obs_number_type *type);

#pragma endregion

#pragma region bgm
//obs_properties_add_bgm_music_list
EXPORT obs_property_t *pls_properties_bgm_add_list(obs_properties_t *props,
						   const char *name,
						   const char *desc);

//obs_property_music_group_clear
EXPORT void pls_property_bgm_list_clear(obs_property_t *p);

EXPORT bool pls_property_bgm_list_clicked(obs_property_t *p, void *context,
					  size_t idx);
//obs_property_music_group_item_count
EXPORT size_t pls_property_bgm_list_item_count(obs_property_t *p);

//obs_property_music_group_add_item
EXPORT size_t pls_property_bgm_list_add_item(obs_property_t *p,
					     const char *name,
					     const char *producer,
					     const char *url, int duration,
					     int duration_type,
					     obs_property_clicked_t callback);

//obs_property_music_group_item_name
EXPORT const char *pls_property_bgm_list_item_name(obs_property_t *p,
						   size_t idx);

//obs_property_music_group_item_producer
EXPORT const char *pls_property_bgm_list_item_producer(obs_property_t *p,
						       size_t idx);

//obs_property_music_group_item_url
EXPORT const char *pls_property_bgm_list_item_url(obs_property_t *p,
						  size_t idx);

//obs_property_music_group_item_duration
EXPORT int pls_property_bgm_list_item_duration(obs_property_t *p, size_t idx);

//obs_property_music_group_item_duration_type
EXPORT int pls_property_bgm_list_item_duration_type(obs_property_t *p,
						    size_t idx);

//obs_properties_add_tips
EXPORT obs_property_t *pls_properties_bgm_add_tips(obs_properties_t *props,
						   const char *name,
						   const char *desc);
#pragma endregion

#pragma region
EXPORT obs_property_t *
//obs_properties_add_chat_template_list
pls_properties_chat_add_template_list(obs_properties_t *props, const char *name,
				      const char *desc);

EXPORT obs_property_t *
//obs_properties_add_chat_font_size
pls_properties_chat_add_font_size(obs_properties_t *props, const char *name,
				  const char *desc, int min, int max, int step);

//obs_property_chat_font_size_min
EXPORT int pls_property_chat_font_size_min(obs_property_t *p);

//obs_property_chat_font_size_max
EXPORT int pls_property_chat_font_size_max(obs_property_t *p);

//obs_property_chat_font_size_step
EXPORT int pls_property_chat_font_size_step(obs_property_t *p);
#pragma endregion

#pragma region
//obs_properties_add_tm_tab
EXPORT obs_property_t *pls_properties_tm_add_tab(obs_properties_t *props,
						 const char *name);

//obs_properties_add_tm_template_tab
EXPORT obs_property_t *
pls_properties_tm_add_template_tab(obs_properties_t *props, const char *name);

//obs_properties_add_tm_template_list
EXPORT obs_property_t *
pls_properties_tm_add_template_list(obs_properties_t *props, const char *name);

//obs_properties_add_tm_template_default_text
EXPORT obs_property_t *
pls_properties_tm_add_template_default_text(obs_properties_t *props,
					    const char *name, const char *desc);

//obs_properties_add_tm_content
EXPORT obs_property_t *pls_properties_tm_add_content(obs_properties_t *props,
						     const char *name,
						     const char *desc);

//obs_properties_add_tm_text
EXPORT obs_property_t *pls_properties_tm_add_text(obs_properties_t *props,
						  const char *name,
						  const char *desc, int min,
						  int max, int step);

//obs_properties_add_tm_color
EXPORT obs_property_t *pls_properties_tm_add_color(obs_properties_t *props,
						   const char *name,
						   const char *desc, int min,
						   int max, int step);

//obs_properties_add_tm_motion
EXPORT obs_property_t *pls_properties_tm_add_motion(obs_properties_t *props,
						    const char *name,
						    const char *desc, int min,
						    int max, int step);

//obs_property_tm_text_min
EXPORT int pls_property_tm_text_min(obs_property_t *p,
				    enum pls_property_type type);

//obs_property_tm_text_max
EXPORT int pls_property_tm_text_max(obs_property_t *p,
				    enum pls_property_type type);

//obs_property_tm_text_step
EXPORT int pls_property_tm_text_step(obs_property_t *p,
				     enum pls_property_type type);
#pragma region

#pragma region image_group
//obs_properties_add_image_group
EXPORT obs_property_t *
pls_properties_add_image_group(obs_properties_t *props, const char *name,
			       const char *desc, int row, int column,
			       enum pls_image_style_type type);

EXPORT void pls_property_image_group_clear(obs_property_t *p);

//obs_property_image_group_clicked
EXPORT bool pls_property_image_group_clicked(obs_property_t *p, void *context,
					     size_t idx);

EXPORT size_t pls_property_image_group_item_count(obs_property_t *p);

//obs_property_image_group_add_item
EXPORT size_t pls_property_image_group_add_item(
	obs_property_t *p, const char *name, const char *url, long long val,
	obs_property_clicked_t callback);

//obs_property_image_group_params
EXPORT bool pls_property_image_group_params(obs_property_t *p, int *row,
					    int *colum,
					    enum pls_image_style_type *type);

//obs_property_image_group_item_name
EXPORT const char *pls_property_image_group_item_name(obs_property_t *p,
						      size_t idx);

//obs_property_image_group_item_url
EXPORT const char *pls_property_image_group_item_url(obs_property_t *p,
						     size_t idx);
#pragma region

#pragma region visualizer custom group
//obs_properties_add_custom_group
EXPORT obs_property_t *
pls_properties_visualizer_add_custom_group(obs_properties_t *props,
					   const char *name, const char *desc,
					   int row, int column);

EXPORT void pls_property_visualizer_custom_group_clear(obs_property_t *p);

EXPORT size_t pls_property_visualizer_custom_group_item_count(obs_property_t *p);

//obs_properties_custom_group_add_int
EXPORT size_t pls_property_visualizer_custom_group_add_int(
	obs_property_t *p, const char *name, const char *desc, int min, int max,
	int step, const char *suffix);

EXPORT size_t pls_property_visualizer_custom_group_add_int_ex(
	obs_property_t *p, const char *name, const char *desc, int min, int max,
	int step, const char *suffix, enum obs_number_type type);

EXPORT size_t pls_property_visualizer_custom_group_add_float(
	obs_property_t *p, const char *name, const char *desc, double min,
	double max, double step, const char *suffix, enum obs_number_type type);

//obs_property_custom_group_row_column
EXPORT bool pls_property_visualizer_custom_group_params(obs_property_t *p,
							int *row, int *colum);

EXPORT enum pls_custom_group_type
pls_property_visualizer_custom_group_item_type(obs_property_t *p, size_t idx);

//obs_property_custom_group_item_name, obs_property_custom_group_item_desc
EXPORT bool pls_property_visualizer_custom_group_item_params(obs_property_t *p,
							     size_t idx,
							     const char **name,
							     const char **desc);

//obs_properties_custom_group_int_params
EXPORT bool pls_property_visualizer_custom_group_item_int_params(
	obs_property_t *p, int *min, int *max, int *step, size_t idx);

//obs_property_custom_group_int_suffix
EXPORT const char *
pls_property_visualizer_custom_group_item_int_suffix(obs_property_t *p,
						     size_t idx);

//obs_properties_custom_group_set_int_params
EXPORT bool pls_property_visualizer_custom_group_item_set_int_params(
	obs_property_t *p, int min, int max, int step, size_t idx);

EXPORT bool pls_property_visualizer_custom_group_item_int_params_ex(
	obs_property_t *p, size_t idx, int *min, int *max, int *step,
	const char **suffix, enum obs_number_type *type);

EXPORT bool pls_property_visualizer_custom_group_item_float_params(
	obs_property_t *p, size_t idx, double *min, double *max, double *step,
	const char **suffix, enum obs_number_type *type);
#pragma region

//obs_properties_add_bool_left
EXPORT obs_property_t *pls_properties_add_bool_left(obs_properties_t *props,
						    const char *name,
						    const char *desc);

#pragma region virtual background
//obs_properties_add_camera_virtual_background_state
EXPORT obs_property_t *pls_properties_virtual_background_add_camera_state(
	obs_properties_t *props, const char *name, const char *desc);

//obs_properties_add_virtual_background_resource
EXPORT obs_property_t *pls_properties_virtual_background_add_resource(
	obs_properties_t *props, const char *name, const char *desc);

//obs_properties_add_switch
EXPORT obs_property_t *pls_properties_virtual_background_add_switch(
	obs_properties_t *props, const char *name, const char *desc);
#pragma region

#pragma region mobile
//obs_properties_add_mobile_help
EXPORT obs_property_t *pls_properties_mobile_add_help(obs_properties_t *props,
						      const char *name,
						      const char *desc,
						      const char *image_url);

EXPORT const char *pls_property_mobile_help_image_url(obs_property_t *p);

//obs_properties_add_mobile_name
EXPORT obs_property_t *
pls_properties_add_mobile_name(obs_properties_t *props, const char *name,
			       const char *desc, const char *value,
			       const char *button_desc, bool button_enabled,
			       obs_property_clicked_t callback);

EXPORT const char *pls_property_mobile_name_value(obs_property_t *p);

EXPORT const char *pls_property_mobile_name_button_desc(obs_property_t *p);

EXPORT bool pls_property_mobile_name_button_enabled(obs_property_t *p);

EXPORT bool pls_property_mobile_name_button_clicked(obs_property_t *p,
						    void *context);

//obs_properties_add_mobile_status
EXPORT obs_property_t *pls_properties_mobile_add_state(obs_properties_t *props,
						       const char *name,
						       const char *desc,
						       const char *value,
						       const char *image_url);

EXPORT const char *pls_property_mobile_state_value(obs_property_t *p);

EXPORT const char *pls_property_mobile_state_image_url(obs_property_t *p);
#pragma region

//obs_properties_add_font_simple
EXPORT obs_property_t *pls_properties_add_font_simple(obs_properties_t *props,
						      const char *name,
						      const char *desc);

//obs_properties_add_color_checkbox
EXPORT obs_property_t *
pls_properties_add_color_checkbox(obs_properties_t *props, const char *name,
				  const char *desc);

//obs_properties_add_audio_meter
EXPORT obs_property_t *pls_properties_add_audio_meter(obs_properties_t *props,
						      const char *name,
						      const char *desc);

//obs_properties_add_template_list
EXPORT obs_property_t *pls_properties_add_template_list(obs_properties_t *props,
							const char *name,
							const char *desc);

//obs_properties_add_color_alpha_checkbox
EXPORT obs_property_t *
pls_properties_add_color_alpha_checkbox(obs_properties_t *props,
					const char *name, const char *desc);

//obs_properties_add_region_select
EXPORT obs_property_t *pls_properties_add_region_select(obs_properties_t *props,
							const char *name,
							const char *desc);

//chat template source pls_properties_add_display

EXPORT obs_property_t *pls_properties_add_display(obs_properties_t *props,
						  const char *name,
						  const char *desc);
//chat template source pls_properties_add_options

EXPORT obs_property_t *pls_properties_add_options(obs_properties_t *props,
						  const char *name,
						  const char *desc);
//chat template source pls_properties_add_motion

EXPORT obs_property_t *pls_properties_add_motion(obs_properties_t *props,
						 const char *name,
						 const char *desc);
//chat template source pls_properties_add_font

EXPORT obs_property_t *pls_properties_add_font(obs_properties_t *props,
					       const char *name,
					       const char *desc);
//chat template source pls_properties_add_text_color

EXPORT obs_property_t *pls_properties_add_text_color(obs_properties_t *props,
						     const char *name,
						     const char *desc);
//chat template source pls_properties_add_bk_color

EXPORT obs_property_t *pls_properties_add_bk_color(obs_properties_t *props,
						   const char *name,
						   const char *desc);
//obs_properties_add_chzzk_sponsor
EXPORT obs_property_t *pls_properties_add_chzzk_sponsor(obs_properties_t *props,
							const char *name);
#ifdef __cplusplus
}
#endif
