#include "pls-obs-api.h"
#include "util/platform.h"
#include "signal.h"
#include "pls-encoder.h"
#include <set>
#include <string>
#include <algorithm>
#include <mutex>
#include <array>
#include <vector>
#include <deque>
#include <limits>
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

	static std::recursive_mutex g_encoder_mutex;
	static std::set<void *> g_encoder_set;
};
#if defined(_WIN32)
Gdiplus::PrivateFontCollection *GLobalVariable::g_privateFontCollection =
	nullptr;
std::mutex GLobalVariable::g_colllectionMutex;
std::atomic_bool GLobalVariable::g_locked = false;
#endif
std::atomic_bool GLobalVariable::g_all_mute = false;
std::atomic_bool GLobalVariable::g_design_mode = false;

std::recursive_mutex GLobalVariable::g_encoder_mutex;
std::set<void *> GLobalVariable::g_encoder_set;
} // namespace

std::set<std::string> g_loaded_dll;
static signal_handler_t *g_freetype_handler = NULL;
static std::atomic_bool g_freetype_needs_reload = false;
static std::deque<std::string>g_freetype_paths;

static std::vector<std::string> g_third_party_plugins;

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

#ifdef _WIN32
typedef void (*record_third_party_plugin)(const char *dllName);
static void *record_third_party_plugin_lib = NULL;
static record_third_party_plugin record_third_party_plugin_func = NULL;

void pls_start_recording_third_party_plugin() {
	char *libdump_path = os_get_executable_path_ptr("libdump-analyzer.dll");
	void *lib = NULL;
	if (!libdump_path) {
		return;
	}
	lib = os_dlopen(libdump_path);
	bfree(libdump_path);
	if (!lib) {
		return;
	}
	record_third_party_plugin_lib = lib;
	record_third_party_plugin func = (record_third_party_plugin)os_dlsym(lib, "record_third_party_plugin");
	if (!func) {
		return;
	}
	record_third_party_plugin_func = func;
}

void pls_record_third_party_plugin(const char *dllName) {
	if (record_third_party_plugin_func) {
		record_third_party_plugin_func(dllName);
	}
}

void pls_finish_recording_third_party_plugin() {
	if (record_third_party_plugin_lib) {
		os_dlclose(record_third_party_plugin_lib);
		record_third_party_plugin_lib = NULL;
	}
	record_third_party_plugin_func = NULL;
}
#endif

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

bool encoder_alive_func(obs_encoder_t *encoder, const char *func_name, int line)
{
	lock_guard guard(GLobalVariable::g_encoder_mutex);
	auto result = GLobalVariable::g_encoder_set.find(encoder) !=
		 GLobalVariable::g_encoder_set.end();
	if (!result) {
		blog(LOG_WARNING,
		     "[%s:%d] Try to access a destroyed encoder: %p", func_name,
		     line, (void *)encoder);
	}

	return result;
}

void on_encoder_created(obs_encoder_t *encoder)
{
	if (!encoder)
		return;

	lock_guard guard(GLobalVariable::g_encoder_mutex);
	if (GLobalVariable::g_encoder_set.find(encoder) ==
	    GLobalVariable::g_encoder_set.end()) {
		GLobalVariable::g_encoder_set.insert(encoder);
	} else {
		assert(false && "Create repeated encoder.");
	}
}

void on_encoder_destoryed(obs_encoder_t *encoder)
{
	lock_guard guard(GLobalVariable::g_encoder_mutex);
	if (auto iter = GLobalVariable::g_encoder_set.find(encoder);
	    iter != GLobalVariable::g_encoder_set.end()) {
		GLobalVariable::g_encoder_set.erase(iter);
	} else {
		assert(false && "Free a destroyed encoder.");
	}
}

#define PTS_STATS_INTERVAL (1000000000LL * 60) // 1min
//#define PTS_STATS_INTERVAL (1000000000LL * 10) // 10s

void init_pts_stats(PtsStats *stats, const obs_encoder_t *encoder,
		    const char *encoder_context_name, const char* encode_info_id)
{
	stats->sum = 0;
	stats->count = 0;
	stats->min = (double)LLONG_MAX;
	stats->max = 0;
	stats->start_time = 0;
	stats->encoder = encoder;
	stats->encoder_context_name = encoder_context_name != NULL
		? encoder_context_name : "unknown";
	stats->encode_info_id = encode_info_id != NULL ? encode_info_id : "unknown";
}


void record_pts_stats(PtsStats *stats, double frame_pts_ms, double pkt_pts_ms)
{
	double diff = frame_pts_ms - pkt_pts_ms;
	uint64_t current = os_gettime_ns();

	if (stats->start_time == 0) {
		stats->start_time = current;
	}

	int64_t elapsed = current - stats->start_time;

	if (elapsed >= PTS_STATS_INTERVAL) {
		double avg = (stats->count == 0
				      ? 0
				      : (double)stats->sum / stats->count);

		blog(LOG_INFO,
		     "Pts stats [encode(%s)(%p)(%s)]: avg=[%.2fms], min=[%.2fms], max=[%.2fms]",
		     stats->encoder_context_name, stats->encoder,
		     stats->encode_info_id, avg, stats->min,
		     stats->max);
		stats->sum = diff;
		stats->count = 1;
		stats->min = diff;
		stats->max = diff;
		stats->start_time = current;
	} else {
		stats->sum += diff;
		stats->count += 1;
		stats->min = min(stats->min, diff);
		stats->max = max(stats->max, diff);
	}
}
