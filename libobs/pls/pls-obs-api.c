#include "pls-obs-api.h"
#include "obs-internal.h"
#include "media-io/audio-io.h"
#include "obs-scene.h"
#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#endif

#define MAX_TEXTURE_SIZE 8192 //D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION

gs_effect_t *pls_effect = NULL;
volatile bool g_app_exiting = false;
volatile bool g_obs_shutdowning = false;

static int g_prism_version_major = 0;
static int g_prism_version_minor = 0;
static int g_prism_version_patch = 0;
static int g_prism_version_build = 0;

static bool pls_source_alive(const obs_source_t *source)
{
	if (!source) {
		return false;
	}

	obs_weak_source_t *control =
		(obs_weak_source_t *)source->context.control;
	if (!control) {
		return false;
	}

	return (control->ref.refs >= 0); // default refs is 0
}

void pls_vst_state_changed(const obs_source_t *source, const char *vst,
			   enum obs_vst_verify_state state)
{
	if (!obs_source_valid(source, "obs_vst_state_changed"))
		return;

	if (!vst)
		return;

	if (!pls_source_alive(source))
		return;

	struct calldata data;
	calldata_init(&data);

	calldata_set_ptr(&data, "source", (void *)source);
	calldata_set_string(&data, "vst_path", vst);
	calldata_set_int(&data, "vst_state", state);

	signal_handler_signal(source->context.signals, "vst_state_changed",
			      &data);

	calldata_free(&data);
}

bool wgc_borderless_enable = false;
void pls_set_wgc_borderless_enable(bool enable)
{
	os_atomic_set_bool(&wgc_borderless_enable, enable);
}

bool pls_get_wgc_borderless_enable()
{
	return os_atomic_load_bool(&wgc_borderless_enable);
}

void pls_source_send_notify(const obs_source_t *source,
			    enum obs_source_event_type type, int sub_code)
{
	if (!source)
		return;

	if (!pls_source_alive(source))
		return;

	struct calldata data;
	uint8_t stack[512];

	calldata_init_fixed(&data, stack, sizeof(stack));
	calldata_set_ptr(&data, "source", (void *)source);
	calldata_set_int(&data, "message", type);
	calldata_set_int(&data, "sub_code", sub_code);

	signal_handler_signal(obs_get_signal_handler(), "source_notify", &data);
}

void pls_source_send_message(const obs_source_t *source,
			     enum obs_source_event_type type,
			     obs_data_t *msg_data)
{
	if (!source)
		return;

	if (!pls_source_alive(source))
		return;

	struct calldata data;
	uint8_t stack[512];

	calldata_init_fixed(&data, stack, sizeof(stack));
	calldata_set_ptr(&data, "source", (void *)source);
	calldata_set_int(&data, "event_type", type);
	calldata_set_ptr(&data, "message", (void *)msg_data);

	signal_handler_signal(obs_get_signal_handler(), "source_message",
			      &data);
}

void pls_source_property_update_notify(const obs_source_t *source,
				       const char *name)
{
	if (!obs_source_valid(source, "obs_source_property_update_notify"))
		return;

	struct calldata data;
	uint8_t stack[128];

	calldata_init_fixed(&data, stack, sizeof(stack));
	calldata_set_ptr(&data, "source", (void *)source);
	calldata_set_string(&data, "name", name);
	signal_handler_signal(source->context.signals, "property_update_notify",
			      &data);
}

void pls_source_cef_received_web_msg(const obs_source_t *source,
				     const char *msg)
{
	if (!obs_source_valid(source, "obs_source_cef_received_web_msg"))
		return;

	if (!source) {
		return;
	}

	if (msg) {
		struct calldata data;
		calldata_init(&data);
		calldata_set_ptr(&data, "source", (void *)source);
		calldata_set_string(&data, "msg", msg);
		signal_handler_signal(source->context.signals,
				      "sub_web_receive", &data);
		calldata_free(&data);
	}
}

void pls_audio_output_get_info(uint32_t *samples_per_sec, int *speakers)
{
	const struct audio_output_info *info =
		audio_output_get_info(obs_get_audio());
	*samples_per_sec = info->samples_per_sec;
	*speakers = info->speakers;
}

void pls_load_sources(obs_data_array_t *array, obs_load_source_cb cb,
		      obs_load_pld_cb pldCb, void *private_data,
		      void *pld_private_data)
{
	struct obs_core_data *data = &obs->data;
	DARRAY(obs_source_t *) sources;
	size_t count;
	size_t i;

	da_init(sources);

	count = obs_data_array_count(array);
	da_reserve(sources, count);

	pthread_mutex_lock(&data->sources_mutex);

	for (i = 0; i < count; i++) {
		obs_data_t *source_data = obs_data_array_item(array, i);
		obs_source_t *source = obs_load_source(source_data);

		da_push_back(sources, &source);

		obs_data_release(source_data);
		if (pldCb)
			pldCb(NULL, source);
	}

	/* tell sources that we want to load */
	for (i = 0; i < sources.num; i++) {
		obs_source_t *source = sources.array[i];
		obs_data_t *source_data = obs_data_array_item(array, i);
		if (source) {
			if (source->info.type == OBS_SOURCE_TYPE_TRANSITION)
				obs_transition_load(source, source_data);
			obs_source_load2(source);
			if (cb)
				cb(private_data, source);
			if (pldCb)
				pldCb(pld_private_data, source);
		}
		obs_data_release(source_data);
	}

	for (i = 0; i < sources.num; i++)
		obs_source_release(sources.array[i]);

	pthread_mutex_unlock(&data->sources_mutex);

	da_free(sources);
}

char *pls_get_module_file_name_ptr(const char *module_name)
{
	UNUSED_PARAMETER(module_name);
#if defined(_WIN32)
	char *ptr;
	wchar_t *path_utf16;
	HMODULE hmodule;
	wchar_t file_name[MAX_PATH];
	struct dstr path;

	if (!os_utf8_to_wcs_ptr(module_name, 0, &path_utf16))
		return NULL;

	hmodule = GetModuleHandleW(path_utf16);
	bfree(path_utf16);

	if (hmodule) {
		GetModuleFileNameW(hmodule, file_name, MAX_PATH);
		os_wcs_to_utf8_ptr(file_name, 0, &ptr);
		dstr_init_move_array(&path, ptr);
		dstr_replace(&path, "\\", "/");
		return path.array;
	}
	return NULL;
#elif defined(__APPLE__)
	return NULL;
#endif
}

//PRISM/Wangshaohui/20220422/#none/load plugin
bool pls_load_plugin(const char *bin_path, const char *data_path)
{
	obs_module_t *module = NULL;

	int code = obs_open_module(&module, bin_path, data_path);
	if (code != MODULE_SUCCESS) {
		blog(LOG_WARNING, "Failed to load given plugin '%s': %d %s",
		     bin_path, code, __FUNCTION__);
		assert(false);
		return false;
	}

	if (!obs_init_module(module)) {
		free_module(module);
		blog(LOG_WARNING, "Failed to init module '%s' %s", bin_path,
		     __FUNCTION__);
		assert(false);
		return false;
	}

	blog(LOG_INFO, "Success to load given plugin '%s' %s", bin_path,
	     __FUNCTION__);
	return true;
}

void pls_scene_update_canvas(obs_scene_t *scene, gs_texture_t *texture,
			     bool save)
{
	UNUSED_PARAMETER(save);

	if (!scene)
		return;

	if (!texture && scene->canvas_texture) {
		obs_enter_graphics();
		gs_texture_destroy(scene->canvas_texture);
		scene->canvas_texture = NULL;
		obs_leave_graphics();
		return;
	}

	obs_enter_graphics();
	uint32_t tex_width = gs_texture_get_width(texture);
	uint32_t tex_height = gs_texture_get_height(texture);
	uint32_t width = scene->canvas_texture
				 ? gs_texture_get_width(scene->canvas_texture)
				 : 0;
	uint32_t height = scene->canvas_texture
				  ? gs_texture_get_height(scene->canvas_texture)
				  : 0;

	enum gs_color_format fmt = gs_texture_get_color_format(texture);

	if (tex_width != width || tex_height != height) {
		if (scene->canvas_texture) {
			gs_texture_destroy(scene->canvas_texture);
			scene->canvas_texture = NULL;
		}
	}

	if (!scene->canvas_texture) {
		scene->canvas_texture = gs_texture_create(
			tex_width, tex_height, fmt, 1, NULL, GS_DYNAMIC);
	}

	gs_copy_texture(scene->canvas_texture, texture);
	obs_leave_graphics();
	return;
}

gs_texture_t *pls_scene_get_canvas(obs_scene_t *scene)
{
	if (!scene)
		return NULL;

	obs_enter_graphics();
	if (!scene->canvas_texture) {
		struct obs_video_info ovi;
		obs_get_video_info(&ovi);
		uint32_t width = ovi.base_width;
		uint32_t height = ovi.base_height;
		scene->canvas_texture = gs_texture_create(
			width, height, GS_BGRA, 1, NULL, GS_DYNAMIC);
	}
	obs_leave_graphics();
	return scene->canvas_texture;
}

void pls_scene_canvas_render(void *data)
{
	struct obs_scene *scene = data;
	if (!obs || !scene->canvas_texture)
		return;

	gs_set_cull_mode(GS_NEITHER);

	gs_blend_state_push();
	gs_enable_blending(true);
#if __APPLE__
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);
	const bool previous = gs_set_linear_srgb(true);
#else
	gs_blend_function(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA);
#endif

	gs_effect_t *effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
	const char *tech_name = "Draw";
	gs_technique_t *tech = gs_effect_get_technique(effect, tech_name);
	gs_eparam_t *param = gs_effect_get_param_by_name(effect, "image");

	gs_technique_begin(tech);
	gs_technique_begin_pass(tech, 0);

	gs_effect_set_texture(param, scene->canvas_texture);
	gs_draw_sprite(scene->canvas_texture, 0, 0, 0);

	gs_technique_end_pass(tech);
	gs_technique_end(tech);

	gs_blend_state_pop();

#if __APPLE__
	gs_set_linear_srgb(previous);
#endif
}

void pls_duplicate_scene_canvas(obs_scene_t *dst, obs_scene_t *src)
{

	if (!obs_source_valid(dst, "duplicate_scene_canvas"))
		return;
	if (!obs_source_valid(src, "duplicate_scene_canvas"))
		return;

	if (!src->canvas_texture)
		return;

	obs_enter_graphics();

	uint32_t tex_width = gs_texture_get_width(src->canvas_texture);
	uint32_t tex_height = gs_texture_get_height(src->canvas_texture);
	uint32_t width = obs_source_get_width(dst->source);
	uint32_t height = obs_source_get_height(dst->source);

	if (dst->canvas_texture &&
	    (tex_width != width || tex_height != height)) {
		gs_texture_destroy(dst->canvas_texture);
		dst->canvas_texture = NULL;
	}

	if (!dst->canvas_texture) {
		dst->canvas_texture = gs_texture_create(
			tex_width, tex_height,
			gs_texture_get_color_format(src->canvas_texture), 1,
			NULL, GS_DYNAMIC);
	}

	gs_copy_texture(dst->canvas_texture, src->canvas_texture);

	obs_leave_graphics();
	return;
}

void pls_source_properties_view_ok_button_enable(obs_source_t *source,
						 bool enable)
{
	if (!obs_source_valid(source,
			      "obs_source_properties_view_ok_button_enable"))
		return;

	struct calldata data;
	uint8_t stack[128];

	calldata_init_fixed(&data, stack, sizeof(stack));
	calldata_set_ptr(&data, "source", source);
	calldata_set_bool(&data, "enable", enable);
	signal_handler_signal(source->context.signals,
			      "update_properties_ok_button_enable", &data);
}

#if defined(_WIN32)
#include <psapi.h>
static void log_module_address(obs_module_t *mod)
{
	if (mod && mod->module) {
		MODULEINFO modInfo;
		ZeroMemory(&modInfo, sizeof(MODULEINFO));
		BOOL result = GetModuleInformation(GetCurrentProcess(),
						   (HMODULE)mod->module,
						   &modInfo,
						   sizeof(MODULEINFO));
		if (result) {
			blog(LOG_INFO, "log module address: %s: %p-%p", mod->file,
			     modInfo.lpBaseOfDll,
			     (BYTE *)modInfo.lpBaseOfDll + modInfo.SizeOfImage);
		}
	}
}
#endif

void pls_log_loaded_modules(log_callback callback)
{
	for (obs_module_t *mod = obs->first_module; !!mod; mod = mod->next) {
		if (callback) {
			bool internal_module = !!os_dlsym(
				mod->module, "obs_is_internal_module");
			callback(mod->file, internal_module);
#if defined(_WIN32)
			log_module_address(mod);
#endif
		}
	}
}

uint64_t pls_texture_get_max_size()
{
	return MAX_TEXTURE_SIZE;
}

gs_effect_t *pls_get_prism_effect()
{
	if (pls_effect)
		return pls_effect;

	char *filename = obs_find_data_file("pls.effect");
	pls_effect = gs_effect_create_from_file(filename, NULL);
	bfree(filename);
	return pls_effect;
}

//PRISM/WuLongyue/20230727/None/codec analog
EXPORT void pls_analog_codec_notify(const char *codec, const char *encodeDecode,
				    bool hw)
{
	if (!codec || !encodeDecode) {
		return;
	}

	struct calldata data;
	uint8_t stack[512];

	calldata_init_fixed(&data, stack, sizeof(stack));
	calldata_set_string(&data, "codec", codec);
	calldata_set_string(&data, "encodeDecode", encodeDecode);
	calldata_set_bool(&data, "hw", hw);

	signal_handler_signal(obs->signals, "analog_codec_notify", &data);
}

//PRISM/Zhongling/20231027/#2902/exit crashed
EXPORT void pls_set_obs_exiting(bool exiting)
{
	os_atomic_set_bool(&g_app_exiting, exiting);
}
EXPORT bool pls_get_obs_exiting()
{
	return os_atomic_load_bool(&g_app_exiting);
}

//PRISM/AiGuanghua/20240624/#5561/source signal shut down crashed
EXPORT void pls_set_obs_shutdowning(bool shutdowning) {
	os_atomic_set_bool(&g_obs_shutdowning, shutdowning);
}

EXPORT bool pls_get_obs_shutdowning() {
	return os_atomic_load_bool(&g_obs_shutdowning);
}

//PRISM/Chengbing/20231108/#/prism version
EXPORT void pls_update_prism_version(int major, int minor, int patch, int build)
{
	g_prism_version_major = major;
	g_prism_version_minor = minor;
	g_prism_version_patch = patch;
	g_prism_version_build = build;
}

//PRISM/Chengbing/20231108/#/prism version
EXPORT int pls_prism_version_major()
{
	return g_prism_version_major;
}

//PRISM/Chengbing/20231108/#/prism version
EXPORT int pls_prism_version_minor()
{
	return g_prism_version_minor;
}

//PRISM/Chengbing/20231108/#/prism version
EXPORT int pls_prism_version_patch()
{
	return g_prism_version_patch;
}

//PRISM/Chengbing/20231108/#/prism version
EXPORT int pls_prism_version_build()
{
	return g_prism_version_build;
}
