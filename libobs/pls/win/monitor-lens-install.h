#pragma once
#include <Windows.h>
#include <string>
#include <optional>
#include "util/windows/win-version.h"

class PLSMonitorLensInstall {
public:
	static bool IsLensSupportActive(win_version_info &version);

	PLSMonitorLensInstall();
	~PLSMonitorLensInstall();

	bool IsLensInstalled(bool &changed);
	bool IsLensSupportActive();

private:
	bool TimeoutCheck();
	void CheckLensInstallState(bool &changed);
	std::wstring ReadRegisterField(HKEY root, LPCWSTR subKey, LPCWSTR valueName);

	std::optional<bool> is_lens_installed;
	std::wstring lens_install_dir;
	win_version_info version = {};
	DWORD64 pre_check_version = 0;
	DWORD64 pre_check_install = 0;
};
