#include "pls-obs-api.h"
#include "util/platform.h"
#include "signal.h"
#include <set>
#include <string>
#include <algorithm>
#include <mutex>
#include <array>
#include <vector>
#include <deque>
#if defined(_WIN32)
#include <Windows.h>
#include <Shlwapi.h>
#include <gdiplus.h>
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Gdiplus.lib")
#endif

using namespace std;

namespace {
struct GLobalVariable {
#if defined(_WIN32)
	static Gdiplus::PrivateFontCollection *g_privateFontCollection;
	static std::mutex g_colllectionMutex;
	static std::atomic_bool g_locked;
#endif
	static std::atomic_bool g_all_mute;
	static std::atomic_bool g_design_mode;
};
#if defined(_WIN32)
Gdiplus::PrivateFontCollection *GLobalVariable::g_privateFontCollection =
	nullptr;
std::mutex GLobalVariable::g_colllectionMutex;
std::atomic_bool GLobalVariable::g_locked = false;
#endif
std::atomic_bool GLobalVariable::g_all_mute = false;
std::atomic_bool GLobalVariable::g_design_mode = false;
} // namespace

std::set<std::string> g_loaded_dll;
static signal_handler_t *g_freetype_handler = NULL;
static std::atomic_bool g_freetype_needs_reload = false;
static std::deque<std::string>g_freetype_paths;

static std::string str_tolower(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(),
		       [](unsigned char c) { return std::tolower(c); });
	return s;
}

bool pls_is_dll_already_loaded(const char *dllName)
{
	if (!dllName)
		return false;

	return (g_loaded_dll.find(std::string(dllName)) != g_loaded_dll.end());
}

void pls_remember_dll_name(const char *dllName)
{
	if (!dllName)
		return;
#if defined(_WIN32)
	g_loaded_dll.insert(str_tolower(std::string(dllName)));
#else
	g_loaded_dll.insert(std::string(dllName));
#endif
}

bool pls_is_plugin_in_black_list(const char *dllName)
{
#if defined(_WIN32)
	/* On Windows: use lower case */
	static std::set<std::string> blackListDll = {
		"obs-websocket-compat.dll"};
#else
	static std::set<std::string> blackListDll = {
		"obs-websocket-compat"};
#endif

	if (dllName && *dllName) {
#if defined(_WIN32)
		return (blackListDll.find(str_tolower(std::string(dllName))) !=
			blackListDll.end());
#else
		return (blackListDll.find(dllName) != blackListDll.end());
#endif
	}

	return false;
}

#if defined(_WIN32)
bool pls_get_enum_timeout_device(wchar_t *buffer, size_t size)
{
	using GetEnumTimeDeviceNameFunc = bool (*)(wchar_t *, size_t);
	HMODULE md = GetModuleHandleW(L"win-dshow.dll");
	if (!md)
		return false;

	auto func = (GetEnumTimeDeviceNameFunc)GetProcAddress(
		md, "pls_enum_timeout_device_name");
	if (!func)
		return false;

	return func(buffer, size);
}

void pls_extract_file_name(const char *full_path, char *out_buf, int buf_len)
{
	if (out_buf) {
		memset(out_buf, 0, buf_len);
	}

	if (!full_path || !out_buf) {
		return;
	}

	LPCSTR name = PathFindFileNameA(full_path);
	snprintf(out_buf, buf_len - 1, "%s", name ? name : "");
}

void pls_init_font_collection()
{
	if (nullptr == GLobalVariable::g_privateFontCollection) {
		GLobalVariable::g_privateFontCollection =
			new Gdiplus::PrivateFontCollection();
	}
}

void pls_destory_font_collection()
{
	if (GLobalVariable::g_privateFontCollection) {
		delete GLobalVariable::g_privateFontCollection;
		GLobalVariable::g_privateFontCollection = nullptr;
	}
}

void pls_enter_font_collection()
{
	if (GLobalVariable::g_locked)
		return;

	GLobalVariable::g_locked = true;
	GLobalVariable::g_colllectionMutex.lock();
}

void pls_leave_font_collection()
{
	if (!GLobalVariable::g_locked)
		return;

	GLobalVariable::g_locked = false;
	GLobalVariable::g_colllectionMutex.unlock();
}

void *pls_get_private_font_collection()
{
	return GLobalVariable::g_privateFontCollection;
}

bool pls_add_font_to_private_collection(const char *font_path)
{
	if (!font_path || !GLobalVariable::g_privateFontCollection)
		return false;

	std::array<wchar_t, MAX_PATH> fontPathw = {0};
	os_utf8_to_wcs(font_path, 0, fontPathw.data(), fontPathw.size());
	if (auto status = GLobalVariable::g_privateFontCollection->AddFontFile(
		    fontPathw.data());
	    status == Gdiplus::Ok)
		return true;

	return false;
}
#endif

EXPORT signal_handler_t* pls_freetype_get_signal() {
	if (g_freetype_handler == NULL) {
		g_freetype_handler = signal_handler_create();
	}
	return g_freetype_handler;
}

void pls_freetype_add_font(const char *font_path) {
	g_freetype_paths.push_back(font_path);
	signal_handler_signal(pls_freetype_get_signal(), "freetype_add_font", NULL);
}

const char* pls_freetype_pop_font_path() {
	if (g_freetype_paths.empty()) {
		return NULL;
	}
	std::string path = g_freetype_paths.front();
	g_freetype_paths.pop_front();
	
	const char* cString = new char[path.size() + 1];
	std::strcpy(const_cast<char*>(cString), path.c_str());
	return cString;
}

bool pls_freetype_needs_reload() {
	return g_freetype_needs_reload.load();
}

void pls_freetype_set_needs_reload(bool reload) {
	g_freetype_needs_reload.store(reload);
}

void pls_set_all_mute(bool mute)
{
	GLobalVariable::g_all_mute.store(mute);
}
bool pls_get_all_mute()
{
	return GLobalVariable::g_all_mute.load();
}

mutex g_mutexActiveOutput;
vector<string> g_lstActiveOutput;

extern "C" void pls_add_active_output(const char* name)
{
	lock_guard _guard(g_mutexActiveOutput);

	g_lstActiveOutput.emplace_back(name);
}

extern "C" void pls_remove_active_output(const char *name)
{
	lock_guard _guard(g_mutexActiveOutput);

	if (auto iter = find(g_lstActiveOutput.begin(), g_lstActiveOutput.end(),
		name);
		g_lstActiveOutput.end() != iter)
	{
		g_lstActiveOutput.erase(iter);
	}
}

size_t pls_get_active_output_count()
{
	lock_guard _guard(g_mutexActiveOutput);

	return g_lstActiveOutput.size();
}

const char *pls_get_active_output_name(size_t index) {
	lock_guard _guard(g_mutexActiveOutput);

	if (index >= g_lstActiveOutput.size())
	{
		return "";
	}

	return g_lstActiveOutput[index].c_str();
}

void pls_set_design_mode(bool enable)
{
	GLobalVariable::g_design_mode = enable;
}

bool pls_design_mode()
{
	return GLobalVariable::g_design_mode;
}
