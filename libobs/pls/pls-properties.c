#include "obs-properties.c"
#include "pls-properties.h"

struct tips_data {
	bool whole_row;
};

struct line_data {
	bool whole_row;
};

struct bool_group_item {
	char *name;
	char *desc;
	char *tooltip;
	bool enabled;
	bool visible;
	obs_property_clicked_t callback;
};

struct bool_group_data {
	DARRAY(struct bool_group_item) items;
};

struct button_group_item {
	char *name;
	char *desc;
	char *tooltip;
	bool enabled;
	bool visible;
	bool highlight;
	obs_property_clicked_t callback;
};

struct button_group_data {
	DARRAY(struct button_group_item) items;
};

struct custom_group_item_button {
	bool highlight;
	obs_property_clicked_t callback;
};

struct custom_group_item_bool {
	obs_property_clicked_t callback;
};

struct custom_group_item_checkbox {
	pls_property_checkbox_clicked_t callback;
};

struct custom_group_item {
	char *name;
	char *desc;
	char *tooltip;
	bool enabled;
	bool visible;
	enum pls_custom_group_type type;
	union {
		struct custom_group_item_button button_item;
		struct custom_group_item_bool bool_item;
		struct custom_group_item_checkbox checkbox_item;
		struct int_data int_item;
		struct float_data float_item;
	};
};

struct custom_group_data {
	DARRAY(struct custom_group_item) items;
};

struct bgm_list_item {
	char *name;
	char *producer;
	char *url;
	int duration;
	int duration_type;
	obs_property_clicked_t callback;
};

struct bgm_list_data {
	DARRAY(struct bgm_list_item) items;
};

struct chat_font_size_data {
	int min;
	int max;
	int step;
};

struct tm_text_data {
	int min;
	int max;
	int step;
};

struct image_group_item {
	char *name;
	char *url;
	long long val;
	obs_property_clicked_t callback;
};

struct image_group_data {
	int row;
	int column;
	enum pls_image_style_type type;
	DARRAY(struct image_group_item) items;
};

struct visualizer_custom_group_item {
	char *name;
	char *desc;
	enum pls_custom_group_type type;
	union {
		struct int_data id;
		struct float_data fd;
	};
};

struct visualizer_custom_group_data {
	int row;
	int column;
	DARRAY(struct visualizer_custom_group_item) items;
};

struct mobile_help_data {
	char *image_url;
};

struct mobile_name_data {
	char *value;
	char *button_desc;
	bool button_enabled;
	obs_property_clicked_t callback;
};

struct mobile_state_data {
	char *value;
	char *image_url;
};

struct text_content_data {
	char *content;
};

#define pls_property_xx_item_xx(name, key, type)                        \
	const char *pls_property_##name##_item_##key(obs_property_t *p, \
						     size_t idx)        \
	{                                                               \
		struct name##_data *data = get_pls_type_data(p, type);  \
                                                                        \
		return (data && idx < data->items.num)                  \
			       ? data->items.array[idx].key             \
			       : NULL;                                  \
	}

static inline size_t get_pls_property_size(enum pls_property_type type)
{
	switch (type) {
	case PLS_PROPERTY_TIPS:
		return sizeof(struct tips_data);
	case PLS_PROPERTY_LINE:
		return sizeof(struct line_data);
	case PLS_PROPERTY_TEXT_CONTENT:
		return sizeof(struct text_content_data);
	case PLS_PROPERTY_BOOL_GROUP:
		return sizeof(struct bool_group_data);
	case PLS_PROPERTY_BUTTON_GROUP:
		return sizeof(struct button_group_data);
	case PLS_PROPERTY_CUSTOM_GROUP:
		return sizeof(struct custom_group_data);
	case PLS_PROPERTY_BGM_LIST:
		return sizeof(struct bgm_list_data);
	case PLS_PROPERTY_CHAT_FONT_SIZE:
		return sizeof(struct chat_font_size_data);
	case PLS_PROPERTY_TM_TEXT:
	case PLS_PROPERTY_TM_COLOR:
	case PLS_PROPERTY_TM_MOTION:
		return sizeof(struct tm_text_data);
	case PLS_PROPERTY_IMAGE_GROUP:
		return sizeof(struct image_group_data);
	case PLS_PROPERTY_VISUALIZER_CUSTOM_GROUP:
		return sizeof(struct visualizer_custom_group_data);
	case PLS_PROPERTY_MOBILE_HELP:
		return sizeof(struct mobile_help_data);
	case PLS_PROPERTY_MOBILE_NAME:
		return sizeof(struct mobile_name_data);
	case PLS_PROPERTY_MOBILE_STATE:
		return sizeof(struct mobile_state_data);
	default:
		return 0;
	}
}

static inline void *get_pls_type_data(struct obs_property *prop,
				      enum pls_property_type type)
{
	return get_type_data(prop, (enum obs_property_type)type);
}

static inline struct obs_property *new_pls_prop(struct obs_properties *props,
						const char *name,
						const char *desc,
						enum pls_property_type type)
{
	size_t data_size = get_pls_property_size(type);
	struct obs_property *p;

	p = bzalloc(sizeof(struct obs_property) + data_size);
	p->parent = props;
	p->enabled = true;
	p->visible = true;
	p->type = type;
	p->name = bstrdup(name);
	p->desc = bstrdup(desc);
	propertes_add(props, p);

	return p;
}

void pls_property_map_erase(obs_property_t *p);
static void pls_property_destroy(struct obs_property *property)
{
	struct mobile_help_data *data_mobile_help;
	struct mobile_name_data *data_mobile_name;
	struct mobile_state_data *data_mobile_state;
	struct text_content_data *data_text_content;

	switch (property->type) {
	case PLS_PROPERTY_BOOL_GROUP:
		pls_property_bool_group_clear(property);
		break;
	case PLS_PROPERTY_BUTTON_GROUP:
		pls_property_button_group_clear(property);
		break;
	case PLS_PROPERTY_BGM_LIST:
		pls_property_bgm_list_clear(property);
		break;
	case PLS_PROPERTY_IMAGE_GROUP:
		pls_property_image_group_clear(property);
		break;
	case PLS_PROPERTY_VISUALIZER_CUSTOM_GROUP:
		pls_property_visualizer_custom_group_clear(property);
		break;
	case PLS_PROPERTY_CUSTOM_GROUP:
		pls_property_custom_group_clear(property);
		break;
	case PLS_PROPERTY_MOBILE_HELP:
		data_mobile_help =
			get_pls_type_data(property, PLS_PROPERTY_MOBILE_HELP);
		if (data_mobile_help) {
			bfree(data_mobile_help->image_url);
		}
		break;
	case PLS_PROPERTY_MOBILE_NAME:
		data_mobile_name =
			get_pls_type_data(property, PLS_PROPERTY_MOBILE_NAME);
		if (data_mobile_name) {
			bfree(data_mobile_name->value);
			bfree(data_mobile_name->button_desc);
		}
		break;
	case PLS_PROPERTY_MOBILE_STATE:
		data_mobile_state =
			get_pls_type_data(property, PLS_PROPERTY_MOBILE_STATE);
		if (data_mobile_state) {
			bfree(data_mobile_state->value);
			bfree(data_mobile_state->image_url);
		}
		break;
	case PLS_PROPERTY_TEXT_CONTENT:
		data_text_content =
			get_pls_type_data(property, PLS_PROPERTY_TEXT_CONTENT);
		if (data_text_content) {
			bfree(data_text_content->content);
		}
		break;
	default:
		break;
	}

	pls_property_map_erase(property);
}

obs_properties_t *pls_properties_create(void)
{
	struct obs_properties *props = obs_properties_create();
	obs_properties_set_param(props, props, pls_properties_destroy);

	return props;
}

void pls_properties_destroy(obs_properties_t *props)
{
	if (props) {
		struct obs_property *p = props->first_property;

		while (p) {
			struct obs_property *next = p->next;
			pls_property_destroy(p);
			p = next;
		}
	}
}

#pragma region common
obs_property_t *pls_properties_add_tips(obs_properties_t *props,
					const char *name, const char *desc)

{
	return pls_properties_add_tips_ex(props, name, desc, false);
}

obs_property_t *pls_properties_add_tips_ex(obs_properties_t *props,
					   const char *name, const char *desc,
					   bool whole_row)
{
	if (!props)
		return NULL;

	struct obs_property *p =
		new_pls_prop(props, name, desc, PLS_PROPERTY_TIPS);

	struct tips_data *data = get_property_data(p);
	data->whole_row = whole_row;

	return p;
}

bool pls_property_tips_whole_row(obs_property_t *p)
{
	struct tips_data *data = get_pls_type_data(p, PLS_PROPERTY_TIPS);

	return data ? data->whole_row : false;
}

obs_property_t *pls_properties_add_line(obs_properties_t *props,
					const char *name, const char *desc)
{
	return pls_properties_add_line_ex(props, name, true);
}

obs_property_t *pls_properties_add_line_ex(obs_properties_t *props,
					   const char *name, bool whole_row)
{
	if (!props)
		return NULL;

	struct obs_property *p =
		new_pls_prop(props, name, NULL, PLS_PROPERTY_LINE);

	struct line_data *data = get_property_data(p);
	data->whole_row = whole_row;

	return p;
}

bool pls_property_line_whole_row(obs_property_t *p)
{
	struct line_data *data = get_pls_type_data(p, PLS_PROPERTY_LINE);

	return data ? data->whole_row : false;
}
#pragma endregion

#pragma region bool_group
obs_property_t *pls_properties_add_bool_group(obs_properties_t *props,
					      const char *name,
					      const char *desc)
{
	if (!props || has_prop(props, name))
		return NULL;

	return new_pls_prop(props, name, desc, PLS_PROPERTY_BOOL_GROUP);
}

void pls_property_bool_group_clear(obs_property_t *p)
{
	struct bool_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_BOOL_GROUP);

	if (data) {
		for (size_t i = 0; i < data->items.num; ++i) {
			struct bool_group_item *item = data->items.array + i;
			bfree(item->name);
			bfree(item->desc);
			bfree(item->tooltip);
		}
		da_free(data->items);
	}
}

bool pls_property_bool_group_clicked(obs_property_t *p, void *context,
				     size_t idx)
{
	struct bool_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_BOOL_GROUP);

	if (!data) {
		return false;
	}

	if (idx >= pls_property_bool_group_item_count(p)) {
		return false;
	}

	struct bool_group_item *item = &data->items.array[idx];
	if (item->callback) {
		obs_properties_t *top = get_topmost_parent(p->parent);
		if (p->priv)
			return item->callback(top, p, p->priv);
		return item->callback(
			top, p,
			context != NULL
				? ((struct obs_context_data *)context)->data
				: NULL);
	}

	return false;
}

size_t pls_property_bool_group_item_count(obs_property_t *p)
{
	struct bool_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_BOOL_GROUP);

	return data ? data->items.num : 0;
}

size_t pls_property_bool_group_add_item(obs_property_t *p, const char *name,
					const char *desc, const char *tooltip,
					obs_property_clicked_t callback)
{
	return pls_property_bool_group_add_item_ex(p, name, desc, tooltip, true,
						   true, callback);
}

size_t pls_property_bool_group_add_item_ex(obs_property_t *p, const char *name,
					   const char *desc,
					   const char *tooltip, bool enabled,
					   bool visible,
					   obs_property_clicked_t callback)
{
	struct bool_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_BOOL_GROUP);

	if (data) {
		struct bool_group_item item = {NULL};

		item.name = bstrdup(name);
		item.desc = bstrdup(desc);
		item.tooltip = bstrdup(tooltip);
		item.enabled = enabled;
		item.visible = visible;
		item.callback = callback;

		return da_push_back(data->items, &item);
	}
	return 0;
}

pls_property_xx_item_xx(bool_group, name, PLS_PROPERTY_BOOL_GROUP);

pls_property_xx_item_xx(bool_group, desc, PLS_PROPERTY_BOOL_GROUP);

pls_property_xx_item_xx(bool_group, tooltip, PLS_PROPERTY_BOOL_GROUP);

bool pls_property_bool_group_item_enabled(obs_property_t *p, size_t idx)
{
	struct bool_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_BOOL_GROUP);

	return (data && idx < data->items.num) ? data->items.array[idx].enabled
					       : false;
}

void pls_property_bool_group_set_item_enabled(obs_property_t *p, size_t idx,
					      bool enabled)
{
	struct bool_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_BOOL_GROUP);

	if (data && idx < data->items.num) {
		data->items.array[idx].enabled = enabled;
	}
}

bool pls_property_bool_group_item_visible(obs_property_t *p, size_t idx)
{
	struct bool_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_BOOL_GROUP);

	return (data && idx < data->items.num) ? data->items.array[idx].visible
					       : false;
}
#pragma endregion

#pragma region button_group
obs_property_t *pls_properties_add_button_group(obs_properties_t *props,
						const char *name,
						const char *desc)
{
	if (!props || has_prop(props, name))
		return NULL;

	return new_pls_prop(props, name, desc, PLS_PROPERTY_BUTTON_GROUP);
}

void pls_property_button_group_clear(obs_property_t *p)
{
	struct button_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_BUTTON_GROUP);

	if (data) {
		for (size_t i = 0; i < data->items.num; ++i) {
			struct button_group_item *item = data->items.array + i;
			bfree(item->name);
			bfree(item->desc);
			bfree(item->tooltip);
		}
		da_free(data->items);
	}
}

bool pls_property_button_group_clicked(obs_property_t *p, void *context,
				       size_t idx)
{
	struct button_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_BUTTON_GROUP);

	if (!data) {
		return false;
	}

	if (idx >= pls_property_button_group_item_count(p)) {
		return false;
	}

	struct button_group_item *item = &data->items.array[idx];
	if (item->callback) {
		obs_properties_t *top = get_topmost_parent(p->parent);
		if (p->priv)
			return item->callback(top, p, p->priv);
		return item->callback(
			top, p,
			context != NULL
				? ((struct obs_context_data *)context)->data
				: NULL);
	}

	return false;
}

size_t pls_property_button_group_item_count(obs_property_t *p)
{
	struct button_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_BUTTON_GROUP);

	return data ? data->items.num : 0;
}

size_t pls_property_button_group_add_item(obs_property_t *p, const char *name,
					  const char *desc, bool enabled,
					  obs_property_clicked_t callback)
{
	return pls_property_button_group_add_item_ex(
		p, name, desc, NULL, enabled, true, false, callback);
}

size_t pls_property_button_group_add_item_ex(obs_property_t *p,
					     const char *name, const char *desc,
					     const char *tooltip, bool enabled,
					     bool visible, bool highlight,
					     obs_property_clicked_t callback)
{
	struct button_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_BUTTON_GROUP);

	if (data) {
		struct button_group_item item = {NULL};

		item.name = bstrdup(name);
		item.desc = bstrdup(desc);
		item.tooltip = bstrdup(tooltip);
		item.enabled = enabled;
		item.visible = visible;
		item.highlight = highlight;
		item.callback = callback;

		return da_push_back(data->items, &item);
	}
	return 0;
}

pls_property_xx_item_xx(button_group, name, PLS_PROPERTY_BUTTON_GROUP);

pls_property_xx_item_xx(button_group, desc, PLS_PROPERTY_BUTTON_GROUP);
;
pls_property_xx_item_xx(button_group, tooltip, PLS_PROPERTY_BUTTON_GROUP);

void pls_property_button_group_set_item_desc(obs_property_t *p, size_t idx,
					     const char *desc)
{
	struct button_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_BUTTON_GROUP);

	if (data && idx < data->items.num) {
		if (NULL != data->items.array[idx].desc) {
			bfree(data->items.array[idx].desc);
		}
		data->items.array[idx].desc = bstrdup(desc);
	}
}

bool pls_property_button_group_item_enabled(obs_property_t *p, size_t idx)
{
	struct button_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_BUTTON_GROUP);

	return (data && idx < data->items.num) ? data->items.array[idx].enabled
					       : false;
}

void pls_property_button_group_set_item_enabled(obs_property_t *p, size_t idx,
						bool enabled)
{
	struct button_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_BUTTON_GROUP);

	if (data && idx < data->items.num) {
		data->items.array[idx].enabled = enabled;
	}
}

void pls_property_button_group_set_item_visible(obs_property_t *p, size_t idx,
						bool visible)
{
	struct button_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_BUTTON_GROUP);

	if (data && idx < data->items.num) {
		data->items.array[idx].visible = visible;
	}
}

bool pls_property_button_group_item_visible(obs_property_t *p, size_t idx)
{
	struct button_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_BUTTON_GROUP);

	return (data && idx < data->items.num) ? data->items.array[idx].visible
					       : false;
}

void pls_property_button_group_set_item_highlight(obs_property_t *p, size_t idx,
						  bool highlight)
{
	struct button_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_BUTTON_GROUP);

	if (data && idx < data->items.num) {
		data->items.array[idx].highlight = highlight;
	}
}

bool pls_property_button_group_item_highlight(obs_property_t *p, size_t idx)
{
	struct button_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_BUTTON_GROUP);

	return (data && idx < data->items.num)
		       ? data->items.array[idx].highlight
		       : false;
}
#pragma endregion

#pragma region int_group
obs_property_t *pls_properties_add_int_group(obs_properties_t *props,
					     const char *name, const char *desc)
{
	return pls_properties_add_custom_group(props, name, desc);
}

size_t pls_property_int_group_item_count(obs_property_t *p)
{
	return pls_property_custom_group_item_count(p);
}

size_t pls_property_int_group_add_item(obs_property_t *p, const char *name,
				       const char *desc, int min, int max,
				       int step)
{
	return pls_property_custom_group_add_int(p, name, desc, NULL, true,
						 true, min, max, step, NULL,
						 OBS_NUMBER_SCROLLER);
}

bool pls_property_int_group_item_params(obs_property_t *p, char **name,
					char **desc, int *min, int *max,
					int *step, size_t idx)
{
	return pls_property_custom_group_item_params(p, idx, name, desc, NULL,
						     NULL, NULL) &&
	       pls_property_custom_group_item_int_params(p, idx, min, max, step,
							 NULL, NULL);
}

#pragma endregion

#pragma region custom group
obs_property_t *pls_properties_add_custom_group(obs_properties_t *props,
						const char *name,
						const char *desc)
{
	if (!props || has_prop(props, name))
		return NULL;

	return new_pls_prop(props, name, desc, PLS_PROPERTY_CUSTOM_GROUP);
}

void pls_property_custom_group_clear(obs_property_t *p)
{
	struct custom_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_CUSTOM_GROUP);

	if (data) {
		for (size_t i = 0; i < data->items.num; ++i) {
			struct custom_group_item *item = data->items.array + i;

			bfree(item->name);
			bfree(item->desc);
			bfree(item->tooltip);

			switch (item->type) {
			case PLS_CUSTOM_GROUP_INT:
				bfree(item->int_item.suffix);
				break;
			case PLS_CUSTOM_GROUP_FLOAT:
				bfree(item->float_item.suffix);
				break;
			default:
				break;
			}
		}
		da_free(data->items);
	}
}

bool pls_property_custom_group_button_clicked(obs_property_t *p, size_t idx,
					      void *context)
{
	struct custom_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_CUSTOM_GROUP);

	if (!data) {
		return false;
	}

	if (idx >= pls_property_custom_group_item_count(p)) {
		return false;
	}

	struct custom_group_item *item = &data->items.array[idx];

	if (PLS_CUSTOM_GROUP_BUTTON != item->type) {
		return false;
	}
	if (!item->button_item.callback) {
		return false;
	}

	obs_properties_t *top = get_topmost_parent(p->parent);
	if (p->priv) {
		return item->button_item.callback(top, p, p->priv);
	}
	return item->button_item.callback(top, p, context);
}

bool pls_property_custom_group_bool_clicked(obs_property_t *p, size_t idx,
					    void *context)
{
	struct custom_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_CUSTOM_GROUP);

	if (!data) {
		return false;
	}

	if (idx >= pls_property_custom_group_item_count(p)) {
		return false;
	}

	struct custom_group_item *item = &data->items.array[idx];

	if (PLS_CUSTOM_GROUP_BOOL != item->type) {
		return false;
	}
	if (!item->bool_item.callback) {
		return false;
	}

	obs_properties_t *top = get_topmost_parent(p->parent);
	if (p->priv) {
		return item->bool_item.callback(top, p, p->priv);
	}
	return item->bool_item.callback(top, p, context);
}

bool pls_property_custom_group_checkbox_clicked(obs_property_t *p, size_t idx,
						bool checked, void *context)
{
	struct custom_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_CUSTOM_GROUP);

	if (!data) {
		return false;
	}

	if (idx >= pls_property_custom_group_item_count(p)) {
		return false;
	}

	struct custom_group_item *item = &data->items.array[idx];

	if (PLS_CUSTOM_GROUP_CHECKBOX != item->type) {
		return false;
	}
	if (!item->checkbox_item.callback) {
		return false;
	}

	obs_properties_t *top = get_topmost_parent(p->parent);
	if (p->priv) {
		return item->checkbox_item.callback(top, p, checked, p->priv);
	}
	return item->checkbox_item.callback(top, p, checked, context);
}

size_t pls_property_custom_group_item_count(obs_property_t *p)
{
	struct custom_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_CUSTOM_GROUP);

	return data ? data->items.num : 0;
}

static void pls_property_custom_group_add_item(struct custom_group_item *item,
					       const char *name,
					       const char *desc,
					       const char *tooltip,
					       bool enabled, bool visible,
					       enum pls_custom_group_type type)
{
	item->name = bstrdup(name);
	item->desc = bstrdup(desc);
	item->tooltip = bstrdup(tooltip);
	item->enabled = enabled;
	item->visible = visible;
	item->type = type;
}

size_t pls_property_custom_group_add_button(obs_property_t *p, const char *name,
					    const char *desc,
					    const char *tooltip, bool enabled,
					    bool visible, bool highlight,
					    obs_property_clicked_t callback)
{
	struct custom_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_CUSTOM_GROUP);

	if (data) {
		struct custom_group_item item = {NULL};

		pls_property_custom_group_add_item(&item, name, desc, tooltip,
						   enabled, visible,
						   PLS_CUSTOM_GROUP_BUTTON);

		item.button_item.highlight = highlight;
		item.button_item.callback = callback;

		return da_push_back(data->items, &item);
	}
	return 0;
}

size_t pls_property_custom_group_add_bool(obs_property_t *p, const char *name,
					  const char *desc, const char *tooltip,
					  bool enabled, bool visible,
					  obs_property_clicked_t callback)
{
	struct custom_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_CUSTOM_GROUP);

	if (data) {
		struct custom_group_item item = {NULL};

		pls_property_custom_group_add_item(&item, name, desc, tooltip,
						   enabled, visible,
						   PLS_CUSTOM_GROUP_BOOL);

		item.bool_item.callback = callback;

		return da_push_back(data->items, &item);
	}
	return 0;
}

size_t
pls_property_custom_group_add_checkbox(obs_property_t *p, const char *name,
				       const char *desc, const char *tooltip,
				       bool enabled, bool visible,
				       pls_property_checkbox_clicked_t callback)
{
	struct custom_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_CUSTOM_GROUP);

	if (data) {
		struct custom_group_item item = {NULL};

		pls_property_custom_group_add_item(&item, name, desc, tooltip,
						   enabled, visible,
						   PLS_CUSTOM_GROUP_CHECKBOX);

		item.checkbox_item.callback = callback;

		return da_push_back(data->items, &item);
	}
	return 0;
}

size_t pls_property_custom_group_add_int(obs_property_t *p, const char *name,
					 const char *desc, const char *tooltip,
					 bool enabled, bool visible, int min,
					 int max, int step, const char *suffix,
					 enum obs_number_type type)
{
	struct custom_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_CUSTOM_GROUP);

	if (data) {
		struct custom_group_item item = {NULL};

		pls_property_custom_group_add_item(&item, name, desc, tooltip,
						   enabled, visible,
						   PLS_CUSTOM_GROUP_INT);

		item.int_item.min = min;
		item.int_item.max = max;
		item.int_item.step = step;
		item.int_item.suffix = bstrdup(suffix);
		item.int_item.type = type;

		return da_push_back(data->items, &item);
	}
	return 0;
}

size_t pls_property_custom_group_add_float(obs_property_t *p, const char *name,
					   const char *desc,
					   const char *tooltip, bool enabled,
					   bool visible, double min, double max,
					   double step, const char *suffix,
					   enum obs_number_type type)
{
	struct custom_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_CUSTOM_GROUP);

	if (data) {
		struct custom_group_item item = {NULL};

		pls_property_custom_group_add_item(&item, name, desc, tooltip,
						   enabled, visible,
						   PLS_CUSTOM_GROUP_FLOAT);

		item.float_item.min = min;
		item.float_item.max = max;
		item.float_item.step = step;
		item.float_item.suffix = bstrdup(suffix);
		item.float_item.type = type;

		return da_push_back(data->items, &item);
	}
	return 0;
}

enum pls_custom_group_type
pls_property_custom_group_item_type(obs_property_t *p, size_t idx)
{
	struct custom_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_CUSTOM_GROUP);

	return (data && idx < data->items.num) ? data->items.array[idx].type
					       : PLS_CUSTOM_GROUP_UNKNOWN;
}

bool pls_property_custom_group_item_params(obs_property_t *p, size_t idx,
					   const char **name, const char **desc,
					   const char **tooltip, bool *enabled,
					   bool *visible)
{
	struct custom_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_CUSTOM_GROUP);

	if (data && idx < data->items.num) {
		struct custom_group_item *item = &data->items.array[idx];

		if (name)
			*name = item->name;
		if (desc)
			*desc = item->desc;
		if (tooltip)
			*tooltip = item->tooltip;
		if (enabled)
			*enabled = item->enabled;
		if (visible)
			*visible = item->visible;

		return true;
	}

	return false;
}

bool pls_property_custom_group_item_button_params(obs_property_t *p, size_t idx,
						  bool *highlight)
{
	struct custom_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_CUSTOM_GROUP);

	if (data && idx < data->items.num) {
		struct custom_group_item *item = &data->items.array[idx];

		if (PLS_CUSTOM_GROUP_BUTTON == item->type) {
			if (highlight)
				*highlight = item->button_item.highlight;

			return true;
		}
	}

	return false;
}

bool pls_property_custom_group_item_int_params(obs_property_t *p, size_t idx,
					       int *min, int *max, int *step,
					       const char **suffix,
					       enum obs_number_type *type)
{
	struct custom_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_CUSTOM_GROUP);

	if (data && idx < data->items.num) {
		struct custom_group_item *item = &data->items.array[idx];

		if (PLS_CUSTOM_GROUP_INT == item->type) {
			if (min)
				*min = item->int_item.min;
			if (max)
				*max = item->int_item.max;
			if (step)
				*step = item->int_item.step;
			if (suffix)
				*suffix = item->int_item.suffix;
			if (type)
				*type = item->int_item.type;

			return true;
		}
	}

	return false;
}

bool pls_property_custom_group_item_float_params(obs_property_t *p, size_t idx,
						 double *min, double *max,
						 double *step,
						 const char **suffix,
						 enum obs_number_type *type)
{
	struct custom_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_CUSTOM_GROUP);

	if (data && idx < data->items.num) {
		struct custom_group_item *item = &data->items.array[idx];

		if (PLS_CUSTOM_GROUP_FLOAT == item->type) {
			if (min)
				*min = item->float_item.min;
			if (max)
				*max = item->float_item.max;
			if (step)
				*step = item->float_item.step;
			if (suffix)
				*suffix = item->float_item.suffix;
			if (type)
				*type = item->float_item.type;

			return true;
		}
	}

	return false;
}
#pragma endregion

#pragma region bgm
obs_property_t *pls_properties_bgm_add_list(obs_properties_t *props,
					    const char *name, const char *desc)
{
	if (!props || has_prop(props, name))
		return NULL;

	return new_pls_prop(props, name, desc, PLS_PROPERTY_BGM_LIST);
}

void pls_property_bgm_list_clear(obs_property_t *p)
{
	struct bgm_list_data *data =
		get_pls_type_data(p, PLS_PROPERTY_BGM_LIST);

	if (data) {
		for (size_t i = 0; i < data->items.num; ++i) {
			struct bgm_list_item *item = data->items.array + i;
			bfree(item->name);
			bfree(item->producer);
			bfree(item->url);
		}
		da_free(data->items);
	}
}

bool pls_property_bgm_list_clicked(obs_property_t *p, void *context, size_t idx)
{
	struct bgm_list_data *data =
		get_pls_type_data(p, PLS_PROPERTY_BGM_LIST);

	if (!data) {
		return false;
	}

	if (idx >= pls_property_bgm_list_item_count(p)) {
		return false;
	}

	struct bgm_list_item *item = &data->items.array[idx];
	if (item->callback) {
		obs_properties_t *top = get_topmost_parent(p->parent);
		if (p->priv)
			return item->callback(top, p, p->priv);
		return item->callback(
			top, p,
			context != NULL
				? ((struct obs_context_data *)context)->data
				: NULL);
	}

	return false;
}

size_t pls_property_bgm_list_item_count(obs_property_t *p)
{
	struct bgm_list_data *data =
		get_pls_type_data(p, PLS_PROPERTY_BGM_LIST);

	return data ? data->items.num : 0;
}

size_t pls_property_bgm_list_add_item(obs_property_t *p, const char *name,
				      const char *producer, const char *url,
				      int duration, int duration_type,
				      obs_property_clicked_t callback)
{
	struct bgm_list_data *data =
		get_pls_type_data(p, PLS_PROPERTY_BGM_LIST);

	if (data) {
		struct bgm_list_item item = {NULL};

		item.name = bstrdup(name);
		item.producer = bstrdup(producer);
		item.url = bstrdup(url);
		item.duration = duration;
		item.duration_type = duration_type;
		item.callback = callback;

		return da_push_back(data->items, &item);
	}
	return 0;
}

pls_property_xx_item_xx(bgm_list, name, PLS_PROPERTY_BGM_LIST);

pls_property_xx_item_xx(bgm_list, producer, PLS_PROPERTY_BGM_LIST);

pls_property_xx_item_xx(bgm_list, url, PLS_PROPERTY_BGM_LIST);

int pls_property_bgm_list_item_duration(obs_property_t *p, size_t idx)
{
	struct bgm_list_data *data =
		get_pls_type_data(p, PLS_PROPERTY_BGM_LIST);

	return (data && idx < data->items.num) ? data->items.array[idx].duration
					       : 0;
}

int pls_property_bgm_list_item_duration_type(obs_property_t *p, size_t idx)
{
	struct bgm_list_data *data =
		get_pls_type_data(p, PLS_PROPERTY_BGM_LIST);

	return (data && idx < data->items.num)
		       ? data->items.array[idx].duration_type
		       : -1;
}

obs_property_t *pls_properties_bgm_add_tips(obs_properties_t *props,
					    const char *name, const char *desc)
{
	if (!props || has_prop(props, name))
		return NULL;

	return new_pls_prop(props, name, desc, PLS_PROPERTY_BGM_TIPS);
}
#pragma endregion

#pragma region
obs_property_t *pls_properties_chat_add_template_list(obs_properties_t *props,
						      const char *name,
						      const char *desc)
{
	if (!props || has_prop(props, name))
		return NULL;

	return new_pls_prop(props, name, desc, PLS_PROPERTY_CHAT_TEMPLATE_LIST);
}

obs_property_t *pls_properties_chat_add_font_size(obs_properties_t *props,
						  const char *name,
						  const char *desc, int min,
						  int max, int step)
{
	if (!props || has_prop(props, name))
		return NULL;

	struct obs_property *p =
		new_pls_prop(props, name, desc, PLS_PROPERTY_CHAT_FONT_SIZE);

	struct chat_font_size_data *data = get_property_data(p);
	data->min = min;
	data->max = max;
	data->step = step;

	return p;
}

int pls_property_chat_font_size_min(obs_property_t *p)
{
	struct chat_font_size_data *data =
		get_pls_type_data(p, PLS_PROPERTY_CHAT_FONT_SIZE);

	return data ? data->min : 0;
}

int pls_property_chat_font_size_max(obs_property_t *p)
{
	struct chat_font_size_data *data =
		get_pls_type_data(p, PLS_PROPERTY_CHAT_FONT_SIZE);

	return data ? data->max : 0;
}

int pls_property_chat_font_size_step(obs_property_t *p)
{
	struct chat_font_size_data *data =
		get_pls_type_data(p, PLS_PROPERTY_CHAT_FONT_SIZE);

	return data ? data->step : 0;
}
#pragma endregion

#pragma region
obs_property_t *pls_properties_tm_add_tab(obs_properties_t *props,
					  const char *name)
{
	if (!props || has_prop(props, name))
		return NULL;

	return new_pls_prop(props, name, NULL, PLS_PROPERTY_TM_TAB);
}

obs_property_t *pls_properties_tm_add_template_tab(obs_properties_t *props,
						   const char *name)
{
	if (!props || has_prop(props, name))
		return NULL;

	return new_pls_prop(props, name, NULL, PLS_PROPERTY_TM_TEMPLATE_TAB);
}

obs_property_t *pls_properties_tm_add_template_list(obs_properties_t *props,
						    const char *name)
{
	if (!props || has_prop(props, name))
		return NULL;

	return new_pls_prop(props, name, NULL, PLS_PROPERTY_TM_TEMPLATE_LIST);
}

obs_property_t *
pls_properties_tm_add_template_default_text(obs_properties_t *props,
					    const char *name, const char *desc)
{
	if (!props || has_prop(props, name))
		return NULL;

	return new_pls_prop(props, name, desc, PLS_PROPERTY_TM_DEFAULT_TEXT);
}

obs_property_t *pls_properties_tm_add_content(obs_properties_t *props,
					      const char *name,
					      const char *desc)
{
	if (!props || has_prop(props, name))
		return NULL;

	return new_pls_prop(props, name, desc, PLS_PROPERTY_TM_TEXT_CONTENT);
}

obs_property_t *pls_properties_tm_add_text(obs_properties_t *props,
					   const char *name, const char *desc,
					   int min, int max, int step)
{
	if (!props || has_prop(props, name))
		return NULL;

	struct obs_property *p =
		new_pls_prop(props, name, desc, PLS_PROPERTY_TM_TEXT);

	struct tm_text_data *data = get_property_data(p);
	data->min = min;
	data->max = max;
	data->step = step;

	return p;
}

obs_property_t *pls_properties_tm_add_color(obs_properties_t *props,
					    const char *name, const char *desc,
					    int min, int max, int step)
{
	if (!props || has_prop(props, name))
		return NULL;

	struct obs_property *p =
		new_pls_prop(props, name, desc, PLS_PROPERTY_TM_COLOR);

	struct tm_text_data *data = get_property_data(p);
	data->min = min;
	data->max = max;
	data->step = step;

	return p;
}

obs_property_t *pls_properties_tm_add_motion(obs_properties_t *props,
					     const char *name, const char *desc,
					     int min, int max, int step)
{
	if (!props || has_prop(props, name))
		return NULL;

	struct obs_property *p =
		new_pls_prop(props, name, desc, PLS_PROPERTY_TM_MOTION);

	struct tm_text_data *data = get_property_data(p);
	data->min = min;
	data->max = max;
	data->step = step;

	return p;
}

int pls_property_tm_text_min(obs_property_t *p, enum pls_property_type type)
{
	struct tm_text_data *data = get_pls_type_data(p, type);
	return data ? data->min : 0;
}

int pls_property_tm_text_max(obs_property_t *p, enum pls_property_type type)
{
	struct tm_text_data *data = get_type_data(p, type);
	return data ? data->max : 0;
}

int pls_property_tm_text_step(obs_property_t *p, enum pls_property_type type)
{
	struct tm_text_data *data = get_type_data(p, type);
	return data ? data->step : 0;
}
#pragma endregion

obs_property_t *pls_properties_add_region_select(obs_properties_t *props,
						 const char *name,
						 const char *desc)
{
	if (!props || has_prop(props, name))
		return NULL;

	return new_pls_prop(props, name, desc, PLS_PROPERTY_REGION_SELECT);
}

#pragma region image_group
obs_property_t *pls_properties_add_image_group(obs_properties_t *props,
					       const char *name,
					       const char *desc, int row,
					       int column,
					       enum pls_image_style_type type)
{
	if (!props || has_prop(props, name))
		return NULL;

	struct obs_property *p =
		new_pls_prop(props, name, desc, PLS_PROPERTY_IMAGE_GROUP);

	struct image_group_data *data = get_property_data(p);
	data->row = row;
	data->column = column;
	data->type = type;

	return p;
}

void pls_property_image_group_clear(obs_property_t *p)
{
	struct image_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_IMAGE_GROUP);

	if (data) {
		for (size_t i = 0; i < data->items.num; ++i) {
			struct image_group_item *item = data->items.array + i;

			bfree(item->name);
			bfree(item->url);
		}
		da_free(data->items);
	}
}

bool pls_property_image_group_clicked(obs_property_t *p, void *context,
				      size_t idx)
{
	struct image_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_IMAGE_GROUP);

	if (!data) {
		return false;
	}

	if (idx >= pls_property_button_group_item_count(p)) {
		return false;
	}

	struct image_group_item *item = &data->items.array[idx];
	if (item->callback) {
		obs_properties_t *top = get_topmost_parent(p->parent);
		if (p->priv)
			return item->callback(top, p, p->priv);
		return item->callback(
			top, p,
			context != NULL
				? ((struct obs_context_data *)context)->data
				: NULL);
	}

	return false;
}

size_t pls_property_image_group_item_count(obs_property_t *p)
{
	struct image_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_IMAGE_GROUP);

	return data ? data->items.num : 0;
}

size_t pls_property_image_group_add_item(obs_property_t *p, const char *name,
					 const char *url, long long val,
					 obs_property_clicked_t callback)
{
	struct image_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_IMAGE_GROUP);

	if (data) {
		struct image_group_item item = { NULL };
			
		item.name = bstrdup(name);
		item.url = bstrdup(url);
		item.val = val;
		item.callback = callback;

		return da_push_back(data->items, &item);
	}
	return 0;
}

bool pls_property_image_group_params(obs_property_t *p, int *row, int *colum,
				     enum pls_image_style_type *type)
{
	struct image_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_IMAGE_GROUP);

	if (data) {
		if (row)
			*row = data->row;
		if (colum)
			*colum = data->column;
		if (type)
			*type = data->type;

		return true;
	}

	return false;
}

pls_property_xx_item_xx(image_group, name, PLS_PROPERTY_IMAGE_GROUP);

pls_property_xx_item_xx(image_group, url, PLS_PROPERTY_IMAGE_GROUP);
#pragma endregion

#pragma region visualizer custom group
obs_property_t *
pls_properties_visualizer_add_custom_group(obs_properties_t *props,
					   const char *name, const char *desc,
					   int row, int column)
{
	if (!props || has_prop(props, name))
		return NULL;

	struct obs_property *p = new_pls_prop(
		props, name, desc, PLS_PROPERTY_VISUALIZER_CUSTOM_GROUP);

	struct visualizer_custom_group_data *data = get_property_data(p);
	data->row = row;
	data->column = column;

	return p;
}

void pls_property_visualizer_custom_group_clear(obs_property_t *p)
{
	struct visualizer_custom_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_VISUALIZER_CUSTOM_GROUP);

	if (data) {
		for (size_t i = 0; i < data->items.num; ++i) {
			struct visualizer_custom_group_item *item =
				data->items.array + i;

			bfree(item->name);
			bfree(item->desc);

			switch (item->type) {
			case PLS_CUSTOM_GROUP_INT:
				bfree(item->id.suffix);
				break;
			case PLS_CUSTOM_GROUP_FLOAT:
				bfree(item->fd.suffix);
				break;
			default:
				break;
			}
		}
		da_free(data->items);
	}
}

size_t pls_property_visualizer_custom_group_item_count(obs_property_t *p)
{
	struct visualizer_custom_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_VISUALIZER_CUSTOM_GROUP);

	return data ? data->items.num : 0;
}

size_t pls_property_visualizer_custom_group_add_int(obs_property_t *p,
						    const char *name,
						    const char *desc, int min,
						    int max, int step,
						    const char *suffix)
{
	return pls_property_visualizer_custom_group_add_int_ex(
		p, name, desc, min, max, step, suffix, OBS_NUMBER_SCROLLER);
}

size_t pls_property_visualizer_custom_group_add_int_ex(
	obs_property_t *p, const char *name, const char *desc, int min, int max,
	int step, const char *suffix, enum obs_number_type type)
{
	struct visualizer_custom_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_VISUALIZER_CUSTOM_GROUP);

	if (data) {
		struct visualizer_custom_group_item item = {NULL};

		item.name = bstrdup(name);
		item.desc = bstrdup(desc);
		item.type = PLS_CUSTOM_GROUP_INT;

		item.id.min = min;
		item.id.max = max;
		item.id.step = step;
		item.id.suffix = bstrdup(suffix);
		item.id.type = type;

		return da_push_back(data->items, &item);
	}
	return 0;
}

size_t pls_property_visualizer_custom_group_add_float(
	obs_property_t *p, const char *name, const char *desc, double min,
	double max, double step, const char *suffix, enum obs_number_type type)
{
	struct visualizer_custom_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_VISUALIZER_CUSTOM_GROUP);

	if (data) {
		struct visualizer_custom_group_item item = {NULL};

		item.name = bstrdup(name);
		item.desc = bstrdup(desc);
		item.type = PLS_CUSTOM_GROUP_FLOAT;

		item.fd.min = min;
		item.fd.max = max;
		item.fd.step = step;
		item.fd.suffix = bstrdup(suffix);
		item.fd.type = type;

		return da_push_back(data->items, &item);
	}
	return 0;
}

bool pls_property_visualizer_custom_group_params(obs_property_t *p, int *row,
						 int *colum)
{
	struct visualizer_custom_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_VISUALIZER_CUSTOM_GROUP);

	if (data) {
		if (row)
			*row = data->row;
		if (colum)
			*colum = data->column;

		return true;
	}

	return false;
}

enum pls_custom_group_type
pls_property_visualizer_custom_group_item_type(obs_property_t *p, size_t idx)
{
	struct visualizer_custom_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_VISUALIZER_CUSTOM_GROUP);

	return (data && idx < data->items.num) ? data->items.array[idx].type
					       : PLS_CUSTOM_GROUP_UNKNOWN;
}

bool pls_property_visualizer_custom_group_item_params(obs_property_t *p,
						      size_t idx,
						      const char **name,
						      const char **desc)
{
	struct visualizer_custom_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_VISUALIZER_CUSTOM_GROUP);

	if (data && idx < data->items.num) {
		struct visualizer_custom_group_item *item =
			&data->items.array[idx];

		if (name)
			*name = item->name;
		if (desc)
			*desc = item->desc;

		return true;
	}

	return false;
}

const char *
pls_property_visualizer_custom_group_item_int_suffix(obs_property_t *p,
						     size_t idx)
{
	struct visualizer_custom_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_VISUALIZER_CUSTOM_GROUP);

	if (data && idx < data->items.num) {
		struct visualizer_custom_group_item *item =
			&data->items.array[idx];
		return item->id.suffix;
	}

	return NULL;
}

bool pls_property_visualizer_custom_group_item_set_int_params(obs_property_t *p,
							      int min, int max,
							      int step,
							      size_t idx)
{
	struct visualizer_custom_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_VISUALIZER_CUSTOM_GROUP);

	if (data && idx < data->items.num) {
		struct visualizer_custom_group_item *item =
			&data->items.array[idx];

		if (PLS_CUSTOM_GROUP_INT == item->type) {
			item->id.min = min;
			item->id.max = max;
			item->id.step = step;

			return true;
		}
	}

	return false;
}

EXPORT bool pls_property_visualizer_custom_group_item_int_params(
	obs_property_t *p, int *min, int *max, int *step, size_t idx)
{
	return pls_property_visualizer_custom_group_item_int_params_ex(
		p, idx, min, max, step, NULL, NULL);
}

bool pls_property_visualizer_custom_group_item_int_params_ex(
	obs_property_t *p, size_t idx, int *min, int *max, int *step,
	const char **suffix, enum obs_number_type *type)
{
	struct visualizer_custom_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_VISUALIZER_CUSTOM_GROUP);

	if (data && idx < data->items.num) {
		struct visualizer_custom_group_item *item =
			&data->items.array[idx];

		if (PLS_CUSTOM_GROUP_INT == item->type) {
			if (min)
				*min = item->id.min;
			if (max)
				*max = item->id.max;
			if (step)
				*step = item->id.step;
			if (suffix)
				*suffix = item->id.suffix;
			if (type)
				*type = item->id.type;

			return true;
		}
	}

	return false;
}

bool pls_property_visualizer_custom_group_item_float_params(
	obs_property_t *p, size_t idx, double *min, double *max, double *step,
	const char **suffix, enum obs_number_type *type)
{
	struct visualizer_custom_group_data *data =
		get_pls_type_data(p, PLS_PROPERTY_VISUALIZER_CUSTOM_GROUP);

	if (data && idx < data->items.num) {
		struct visualizer_custom_group_item *item =
			&data->items.array[idx];

		if (PLS_CUSTOM_GROUP_FLOAT == item->type) {
			if (min)
				*min = item->fd.min;
			if (max)
				*max = item->fd.max;
			if (step)
				*step = item->fd.step;
			if (suffix)
				*suffix = item->fd.suffix;
			if (type)
				*type = item->fd.type;

			return true;
		}
	}

	return false;
}
#pragma endregion

obs_property_t *pls_properties_add_bool_left(obs_properties_t *props,
					     const char *name, const char *desc)
{
	if (!props || has_prop(props, name))
		return NULL;

	return new_pls_prop(props, name, desc, PLS_PROPERTY_BOOL_LEFT);
}

#pragma region virtual background
obs_property_t *pls_properties_virtual_background_add_camera_state(
	obs_properties_t *props, const char *name, const char *desc)
{
	if (!props || has_prop(props, name))
		return NULL;

	return new_pls_prop(props, name, desc,
			    PLS_PROPERTY_VIRTUAL_BACKGROUND_CAMERA_STATE);
}

obs_property_t *pls_properties_virtual_background_add_resource(
	obs_properties_t *props, const char *name, const char *desc)
{
	if (!props || has_prop(props, name))
		return NULL;

	return new_pls_prop(props, name, desc,
			    PLS_PROPERTY_VIRTUAL_BACKGROUND_RESOURCE);
}

obs_property_t *
pls_properties_virtual_background_add_switch(obs_properties_t *props,
					     const char *name, const char *desc)
{
	if (!props || has_prop(props, name))
		return NULL;

	return new_pls_prop(props, name, desc,
			    PLS_PROPERTY_VIRTUAL_BACKGROUND_SWITCH);
}
#pragma endregion

#pragma region mobile
obs_property_t *pls_properties_mobile_add_help(obs_properties_t *props,
					       const char *name,
					       const char *desc,
					       const char *image_url)
{
	if (!props)
		return NULL;

	struct obs_property *p =
		new_pls_prop(props, name, desc, PLS_PROPERTY_MOBILE_HELP);

	struct mobile_help_data *data = get_property_data(p);
	data->image_url = bstrdup(image_url);

	return p;
}

const char *pls_property_mobile_help_image_url(obs_property_t *p)
{
	struct mobile_help_data *data =
		get_pls_type_data(p, PLS_PROPERTY_MOBILE_HELP);

	return data ? data->image_url : NULL;
}

obs_property_t *
pls_properties_add_mobile_name(obs_properties_t *props, const char *name,
			       const char *desc, const char *value,
			       const char *button_desc, bool button_enabled,
			       obs_property_clicked_t callback)
{
	if (!props)
		return NULL;

	struct obs_property *p =
		new_pls_prop(props, name, desc, PLS_PROPERTY_MOBILE_NAME);

	struct mobile_name_data *data = get_property_data(p);
	data->value = bstrdup(value);
	data->button_desc = bstrdup(button_desc);
	data->button_enabled = button_enabled;
	data->callback = callback;

	return p;
}

const char *pls_property_mobile_name_value(obs_property_t *p)
{
	struct mobile_name_data *data =
		get_pls_type_data(p, PLS_PROPERTY_MOBILE_NAME);

	return data ? data->value : NULL;
}

const char *pls_property_mobile_name_button_desc(obs_property_t *p)
{
	struct mobile_name_data *data =
		get_pls_type_data(p, PLS_PROPERTY_MOBILE_NAME);

	return data ? data->button_desc : NULL;
}

bool pls_property_mobile_name_button_enabled(obs_property_t *p)
{
	struct mobile_name_data *data =
		get_pls_type_data(p, PLS_PROPERTY_MOBILE_NAME);

	return data ? data->button_enabled : false;
}

bool pls_property_mobile_name_button_clicked(obs_property_t *p, void *context)
{
	struct mobile_name_data *data =
		get_pls_type_data(p, PLS_PROPERTY_MOBILE_NAME);

	if (!data) {
		return false;
	}

	if (data->callback) {
		obs_properties_t *top = get_topmost_parent(p->parent);
		if (p->priv)
			return data->callback(top, p, p->priv);
		return data->callback(
			top, p,
			context != NULL
				? ((struct obs_context_data *)context)->data
				: NULL);
	}

	return false;
}

obs_property_t *pls_properties_mobile_add_state(obs_properties_t *props,
						const char *name,
						const char *desc,
						const char *value,
						const char *image_url)
{
	if (!props)
		return NULL;

	struct obs_property *p =
		new_pls_prop(props, name, desc, PLS_PROPERTY_MOBILE_STATE);

	struct mobile_state_data *data = get_property_data(p);
	data->value = bstrdup(value);
	data->image_url = bstrdup(image_url);

	return p;
}

const char *pls_property_mobile_state_value(obs_property_t *p)
{
	struct mobile_state_data *data =
		get_pls_type_data(p, PLS_PROPERTY_MOBILE_STATE);

	return data ? data->value : NULL;
}

const char *pls_property_mobile_state_image_url(obs_property_t *p)
{
	struct mobile_state_data *data =
		get_pls_type_data(p, PLS_PROPERTY_MOBILE_STATE);

	return data ? data->image_url : NULL;
}

#pragma endregion

obs_property_t *pls_properties_add_font_simple(obs_properties_t *props,
					       const char *name,
					       const char *desc)
{
	if (!props || has_prop(props, name))
		return NULL;

	return new_pls_prop(props, name, desc, PLS_PROPERTY_FONT_SIMPLE);
}

obs_property_t *pls_properties_add_color_checkbox(obs_properties_t *props,
						  const char *name,
						  const char *desc)
{
	if (!props || has_prop(props, name))
		return NULL;

	return new_pls_prop(props, name, desc, PLS_PROPERTY_COLOR_CHECKBOX);
}

obs_property_t *pls_properties_add_audio_meter(obs_properties_t *props,
					       const char *name,
					       const char *desc)
{
	if (!props || has_prop(props, name))
		return NULL;

	return new_pls_prop(props, name, desc, PLS_PROPERTY_AUDIO_METER);
}

obs_property_t *pls_properties_add_template_list(obs_properties_t *props,
						 const char *name,
						 const char *desc)
{
	if (!props || has_prop(props, name))
		return NULL;

	return new_pls_prop(props, name, desc, PLS_PROPERTY_TEMPLATE_LIST);
}

obs_property_t *pls_properties_add_color_alpha_checkbox(obs_properties_t *props,
							const char *name,
							const char *desc)
{
	if (!props || has_prop(props, name))
		return NULL;

	return new_pls_prop(props, name, desc,
			    PLS_PROPERTY_COLOR_ALPHA_CHECKBOX);
}

obs_property_t *pls_properties_add_text_content(obs_properties_t *props,
						const char *name,
						const char *desc,
						const char *content)
{
	if (!props)
		return NULL;

	struct obs_property *p =
		new_pls_prop(props, name, desc, PLS_PROPERTY_TEXT_CONTENT);

	struct text_content_data *data = get_property_data(p);
	data->content = bstrdup(content);

	return p;
}

const char *pls_property_get_text_content(obs_property_t *p)
{
	struct text_content_data *data =
		get_pls_type_data(p, PLS_PROPERTY_TEXT_CONTENT);

	return data ? data->content : NULL;
}
