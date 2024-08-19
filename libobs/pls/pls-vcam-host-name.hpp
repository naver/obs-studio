#if defined(_WIN32)
#pragma once
#include <Windows.h>
#include <string>
#include <array>
#include <shlobj_core.h>
#include <atomic>
#include <strsafe.h>

#define VIRTUAL_CAM_HOST_PROC_FOLDER_NAME_W L"prism_host_proc"
#define VIRTUAL_CAM_HOST_PROC_FOLDER_NAME "prism_host_proc"
#define VIRTUAL_CAM_HOST_PROC_PATH_W L"\\PRISMLiveStudio\\"
static std::atomic_bool g_host_name_logged = false;

namespace vcam {

static int CreateFolder(const std::wstring &folder)
{
	DWORD last_error;
	if (CreateDirectory(folder.c_str(), NULL)) {
		return 0;
	} else if (last_error = GetLastError();
		   ERROR_ALREADY_EXISTS == last_error) {
		return 0;
	}

	return last_error;
}

static std::wstring GetDataPath()
{
	std::array<wchar_t, MAX_PATH> data_path;
	SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT,
			 data_path.data());
	std::wstring path(data_path.data());
	path.append(VIRTUAL_CAM_HOST_PROC_PATH_W)
		.append(VIRTUAL_CAM_HOST_PROC_FOLDER_NAME_W)
		.append(L"\\");
	return path;
}

bool LogHostProcessName(const std::wstring &executeFile)
{
	if (executeFile.empty())
		return false;

	const wchar_t *pName = wcsrchr(executeFile.c_str(), '\\');
	if (pName == NULL) {
		pName = executeFile.c_str();
	} else {
		pName++;
	}
	std::array<wchar_t, MAX_PATH> appName;
	(void)StringCchCopy(appName.data(), appName.size(), pName);

	auto data_path = GetDataPath();
	CreateFolder(data_path);

	/* create a txt file named by host process name */
	HANDLE hFile = CreateFileW(
		data_path.append(appName.data()).append(L".txt").c_str(),
		GENERIC_READ, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile) {
		CloseHandle(hFile);
		return true;
	}

	if (hFile == INVALID_HANDLE_VALUE &&
	    GetLastError() == ERROR_FILE_EXISTS)
		return true;

	return false;
}
} // namespace vcam

#endif
