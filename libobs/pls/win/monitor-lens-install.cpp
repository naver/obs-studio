#include "monitor-lens-install.h"
#include "obs.h"

bool PLSMonitorLensInstall::IsLensSupportActive(win_version_info &version)
{
	static win_version_info lens_version = {};
	if (lens_version.major != version.major || lens_version.minor != version.minor ||
	    lens_version.build != version.build) {
		memcpy(&lens_version, &version, sizeof(win_version_info));
		blog(LOG_INFO, "PRISM Lens version: %d.%d.%d.%d", version.major, version.minor, version.build,
		     version.revis);
	}

	if (version.major > 2 || (version.major == 2 && (version.minor > 0 || version.build > 2)))
		return true; // version > 2.0.2

	return false;
}

PLSMonitorLensInstall::PLSMonitorLensInstall()
{
	bool changed = false;
	CheckLensInstallState(changed);
}

PLSMonitorLensInstall::~PLSMonitorLensInstall() {}

bool PLSMonitorLensInstall::IsLensInstalled(bool &changed)
{
	changed = false;
	if (TimeoutCheck()) {
		CheckLensInstallState(changed);
	}

	return is_lens_installed.value();
}

bool PLSMonitorLensInstall::IsLensSupportActive()
{
	if (!is_lens_installed.has_value()) {
		assert(false);
		return false;
	}

	if (!is_lens_installed.value())
		return false; // not install lens

	if (lens_install_dir.empty())
		return false;

	auto tm = GetTickCount64();
	if (tm - pre_check_version >= 5000) { // check every 5000 milliseconds
		pre_check_version = tm;

		auto str = ReadRegisterField(HKEY_CURRENT_USER,
					     L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\PRISM Lens",
					     L"DisplayVersion");
		assert(!str.empty());
		if (!str.empty()) {
			int vals = swscanf(str.c_str(), L"%d.%d.%d.%d", &version.major, &version.minor, &version.build,
					   &version.revis);
			assert(vals == 4);
			if (vals == 4) {
				return PLSMonitorLensInstall::IsLensSupportActive(version);
			}
		}

		std::wstring path = lens_install_dir + L"\\bin\\64bit\\PRISMLens.exe";
		if (!get_dll_ver(path.c_str(), &version)) {
			auto path = lens_install_dir + L"\\PRISMLens.exe";
			if (!get_dll_ver(path.c_str(), &version)) {
				version = {};
				assert(false);
				return false;
			}
		}
	}

	return PLSMonitorLensInstall::IsLensSupportActive(version);
}

bool PLSMonitorLensInstall::TimeoutCheck()
{
	auto tm = GetTickCount64() - pre_check_install;
	return tm >= 500; // check every 500 milliseconds
}

void PLSMonitorLensInstall::CheckLensInstallState(bool &changed)
{
	lens_install_dir =
		ReadRegisterField(HKEY_CURRENT_USER, L"Software\\NAVER Corporation\\PRISM Lens", L"InstallDir");
	bool installed = false == lens_install_dir.empty();
	if (!is_lens_installed.has_value() || installed != is_lens_installed.value()) {
		changed = true;
		pre_check_version = 0;
		is_lens_installed = installed;
		blog(LOG_INFO, "%s lens installed: %s", __FUNCTION__, installed ? "yes" : "no");
	}

	pre_check_install = GetTickCount64();
}

std::wstring PLSMonitorLensInstall::ReadRegisterField(HKEY root, LPCWSTR subKey, LPCWSTR valueName)
{
	HKEY hKey = nullptr;
	LONG result = RegOpenKeyExW(root, subKey, 0, KEY_READ, &hKey);
	if (result != ERROR_SUCCESS) {
		return L"";
	}

	DWORD dataType = 0;
	DWORD dataSize = 0;
	result = RegQueryValueExW(hKey, valueName, nullptr, &dataType, nullptr, &dataSize);

	if (result != ERROR_SUCCESS) {
		RegCloseKey(hKey);
		return L"";
	}

	if (dataType != REG_SZ && dataType != REG_EXPAND_SZ) {
		RegCloseKey(hKey);
		return L"";
	}

	std::wstring value;
	value.resize(dataSize / sizeof(wchar_t));
	result = RegQueryValueExW(hKey, valueName, nullptr, &dataType, reinterpret_cast<LPBYTE>(&value[0]), &dataSize);

	RegCloseKey(hKey);

	if (result != ERROR_SUCCESS) {
		return L"";
	}

	size_t nullPos = value.find(L'\0');
	if (nullPos != std::wstring::npos) {
		value.resize(nullPos);
	}

	return value;
}
