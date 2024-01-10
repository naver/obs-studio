/*****************************************************************************
Copyright (C) 2016-2017 by Colin Edwards.
Additional Code Copyright (C) 2016-2017 by c3r1c3 <c3r1c3@nevermindonline.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#include "headers/VSTPlugin.h"
#include <QRegularExpression>
#include <pls/pls-source.h>
#include <obs-module.h>

#define OPEN_VST_SETTINGS "open_vst_settings"
#define OPEN_WHEN_ACTIVE_VST_SETTINGS "open_when_active_vst_settings"
#define PLUGIN_PATH_VST_SETTINGS "plugin_path"

#define PLUG_IN_NAME obs_module_text("VstPlugin")
#define OPEN_VST_TEXT obs_module_text("OpenPluginInterface")
#define OPEN_WHEN_ACTIVE_VST_TEXT obs_module_text("OpenInterfaceWhenActive")
#define SELECT_A_PLUG_IN_TEXT obs_module_text("SelectPlugin")

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-vst", "en-US")

static bool open_editor_button_clicked(obs_properties_t *props,
				       obs_property_t *property, void *data)
{
	VSTPlugin *vstPlugin = (VSTPlugin *)data;
	QMetaObject::invokeMethod(vstPlugin, "openEditor");

	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(property);
	UNUSED_PARAMETER(data);

	return true;
}

static const char *vst_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return PLUG_IN_NAME;
}

static void vst_destroy(void *data)
{
	VSTPlugin *vstPlugin = (VSTPlugin *)data;
	QMetaObject::invokeMethod(vstPlugin, "closeEditor");
	vstPlugin->NotifyStop();
	vstPlugin->deleteLater();
}

static void vst_update(void *data, obs_data_t *settings)
{
	VSTPlugin *vstPlugin = (VSTPlugin *)data;

	bool openInterfaceWhenActive =
		obs_data_get_bool(settings, OPEN_WHEN_ACTIVE_VST_SETTINGS);
	const char *path =
		obs_data_get_string(settings, PLUGIN_PATH_VST_SETTINGS);

	if (path && (*path))
		vstPlugin->updateVst(std::string(path),
				     openInterfaceWhenActive);
	else
		vstPlugin->clearVst();
}

const char *vst_signal =
	"void vst_state_changed(ptr vst_source, string vst_name, int state)";

static void *vst_create(obs_data_t *settings, obs_source_t *filter)
{
	VSTPlugin *vstPlugin = new VSTPlugin(filter);
	signal_handler_add(obs_source_get_signal_handler(filter), vst_signal);
	vst_update(vstPlugin, settings);
	return vstPlugin;
}

static void vst_save(void *data, obs_data_t *settings)
{
	VSTPlugin *vstPlugin = (VSTPlugin *)data;
	obs_data_set_string(settings, "chunk_data",
			    vstPlugin->getChunk().c_str());
	obs_data_set_string(settings, "chunk_hash",
			    vstPlugin->getUsingVstHash().c_str());
}

static struct obs_audio_data *vst_filter_audio(void *data,
					       struct obs_audio_data *audio)
{
	VSTPlugin *vstPlugin = (VSTPlugin *)data;
	vstPlugin->process(audio);

	/*
	 * OBS can only guarantee getting the filter source's parent and own name
	 * in this call, so we grab it and return the results for processing
	 * by the EditorWidget.
	 */
	vstPlugin->getSourceNames();

	return audio;
}

static void fill_out_plugins(obs_property_t *list)
{
	QStringList dir_list;

#ifdef __APPLE__
	dir_list << "/Library/Audio/Plug-Ins/VST/"
		 << "~/Library/Audio/Plug-ins/VST/";
#elif WIN32
#ifndef _WIN64
	HANDLE hProcess = GetCurrentProcess();

	BOOL isWow64;
	IsWow64Process(hProcess, &isWow64);

	if (!isWow64) {
#endif
		dir_list << qEnvironmentVariable("ProgramFiles") +
				    "/Steinberg/VstPlugins/"
			 << qEnvironmentVariable("CommonProgramFiles") +
				    "/Steinberg/Shared Components/"
			 << qEnvironmentVariable("CommonProgramFiles") + "/VST2"
			 << qEnvironmentVariable("CommonProgramFiles") +
				    "/Steinberg/VST2"
			 << qEnvironmentVariable("CommonProgramFiles") +
				    "/VSTPlugins/"
			 << qEnvironmentVariable("ProgramFiles") +
				    "/VSTPlugins/";
#ifndef _WIN64
	} else {
		dir_list << qEnvironmentVariable("ProgramFiles(x86)") +
				    "/Steinberg/VstPlugins/"
			 << qEnvironmentVariable("CommonProgramFiles(x86)") +
				    "/Steinberg/Shared Components/"
			 << qEnvironmentVariable("CommonProgramFiles(x86)") +
				    "/VST2"
			 << qEnvironmentVariable("CommonProgramFiles(x86)") +
				    "/VSTPlugins/"
			 << qEnvironmentVariable("ProgramFiles(x86)") +
				    "/VSTPlugins/";
	}
#endif
#elif __linux__
	// If the user has set the VST_PATH environmental
	// variable, then use it. Else default to a list
	// of common locations.
	char *vstPathEnv;
	vstPathEnv = getenv("VST_PATH");
	if (vstPathEnv != nullptr) {
		dir_list << vstPathEnv;
	} else {
		// Choose the most common locations
		dir_list << "/usr/lib/vst/"
			 << "/usr/lib/lxvst/"
			 << "/usr/lib/linux_vst/"
			 << "/usr/lib64/vst/"
			 << "/usr/lib64/lxvst/"
			 << "/usr/lib64/linux_vst/"
			 << "/usr/local/lib/vst/"
			 << "/usr/local/lib/lxvst/"
			 << "/usr/local/lib/linux_vst/"
			 << "/usr/local/lib64/vst/"
			 << "/usr/local/lib64/lxvst/"
			 << "/usr/local/lib64/linux_vst/"
			 << "~/.vst/"
			 << "~/.lxvst/";
	}
#endif

	QStringList filters;

#ifdef __APPLE__
	filters << "*.vst";
#elif WIN32
	filters << "*.dll";
#elif __linux__
	filters << "*.so"
		<< "*.o";
#endif

	QStringList vst_list;

	// Read all plugins into a list...
	for (int a = 0; a < dir_list.size(); ++a) {
		QDir search_dir(dir_list[a]);
		search_dir.setNameFilters(filters);
		QDirIterator it(search_dir, QDirIterator::Subdirectories);
		while (it.hasNext()) {
			QString path = it.next();
			QString name = it.fileName();

#ifdef __APPLE__
			name.remove(QRegularExpression("(\\.vst)"));
#elif WIN32
			name.remove(QRegularExpression("(\\.dll)"));
#elif __linux__
			name.remove(QRegularExpression("(\\.so|\\.o)"));
#endif

			name.append("=").append(path);
			vst_list << name;
		}
	}

	// Now sort list alphabetically (still case-sensitive though).
	std::stable_sort(vst_list.begin(), vst_list.end(),
			 std::less<QString>());

	std::string text =
		std::string("{").append(SELECT_A_PLUG_IN_TEXT).append("}");
	obs_property_list_add_string(list, text.c_str(), nullptr);

	for (int b = 0; b < vst_list.size(); ++b) {
		QString vst_sorted = vst_list[b];
		obs_property_list_add_string(
			list,
			vst_sorted.left(vst_sorted.indexOf('='))
				.toStdString()
				.c_str(),
			vst_sorted.mid(vst_sorted.indexOf('=') + 1)
				.toStdString()
				.c_str());
	}
}

static obs_properties_t *vst_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_property_t *list = obs_properties_add_list(
		props, PLUGIN_PATH_VST_SETTINGS, PLUG_IN_NAME,
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	fill_out_plugins(list);

	obs_properties_add_button(props, OPEN_VST_SETTINGS, OPEN_VST_TEXT,
				  open_editor_button_clicked);
	obs_properties_add_bool(props, OPEN_WHEN_ACTIVE_VST_SETTINGS,
				OPEN_WHEN_ACTIVE_VST_TEXT);

	UNUSED_PARAMETER(data);
	return props;
}

static void vst_get_private_data(void *obj, obs_data_t *data)
{
	if (!obj || !data)
		return;

	const char *method = obs_data_get_string(data, "method");
	if (!method)
		return;

	VSTPlugin *vstPlugin = reinterpret_cast<VSTPlugin *>(obj);
	if (0 == strcmp(method, "get_vst_state")) {
		std::string vst;
		enum obs_vst_verify_state state;
		vstPlugin->getVstState(vst, state);

		obs_data_set_string(data, "vst_path", vst.c_str());
		obs_data_set_int(data, "vst_state", state);
		return;

	} else {
		assert(false);
		return;
	}
}

bool obs_module_load(void)
{
	struct obs_source_info vst_filter = {};
	vst_filter.id = "vst_filter";
	vst_filter.type = OBS_SOURCE_TYPE_FILTER;
	vst_filter.output_flags = OBS_SOURCE_AUDIO;
	vst_filter.get_name = vst_name;
	vst_filter.create = vst_create;
	vst_filter.destroy = vst_destroy;
	vst_filter.update = vst_update;
	vst_filter.filter_audio = vst_filter_audio;
	vst_filter.get_properties = vst_properties;
	vst_filter.save = vst_save;

	struct pls_source_info private_info = {};
	private_info.get_private_data = vst_get_private_data;

	register_pls_source_info(&vst_filter, &private_info);
    obs_register_source(&vst_filter);
	return true;
}