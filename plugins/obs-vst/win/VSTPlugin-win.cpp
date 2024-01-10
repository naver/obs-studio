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
#include "../headers/VSTPlugin.h"
#include <util/platform.h>
#include <windows.h>

typedef AEffect *(*vstPluginMain)(audioMasterCallback audioMaster);
AEffect *VSTPlugin::loadEffect(const std::string &path)
{
	AEffect *plugin = nullptr;

	auto fileName = vst::getFileName(path.c_str());

	wchar_t *wpath;
	os_utf8_to_wcs_ptr(path.c_str(), 0, &wpath);
	dllHandle = LoadLibraryW(wpath);
	bfree(wpath);
	if (dllHandle == nullptr) {
		DWORD errorCode = GetLastError();
		if (errorCode == ERROR_BAD_EXE_FORMAT) {
			warn("Could not open library, wrong architecture. '%s'",
			     qUtf8Printable(fileName));
		} else {
			warn("Failed trying to load VST from '%s', error: %u \n",
			     qUtf8Printable(fileName), errorCode);
		}
		return nullptr;
	}

	vstPluginMain mainEntryPoint =
		(vstPluginMain)GetProcAddress(dllHandle, "VSTPluginMain");

	if (mainEntryPoint == nullptr) {
		mainEntryPoint = (vstPluginMain)GetProcAddress(
			dllHandle, "VstPluginMain()");
	}

	if (mainEntryPoint == nullptr) {
		mainEntryPoint =
			(vstPluginMain)GetProcAddress(dllHandle, "main");
	}

	if (mainEntryPoint == nullptr) {
		warn("Couldn't get a pointer to plug-in's main() '%s'",
		     qUtf8Printable(fileName));
		return nullptr;
	}

	try {
		plugin = mainEntryPoint(hostCallback_static);
	} catch (...) {
		warn("VST plugin initialization failed '%s'",
		     qUtf8Printable(fileName));
		return nullptr;
	}

	if (!plugin) {
		warn("mainEntryPoint failed for '%s'",
		     qUtf8Printable(fileName));
		return nullptr;
	}

	plugin->user = this;
	return plugin;
}

void VSTPlugin::unloadLibrary()
{
	if (dllHandle) {
		FreeLibrary(dllHandle);
		dllHandle = nullptr;
	}
}
