#include "pls-lens-info.h"
#include "pls/pls-source.h"
#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#include <mutex>
#include <array>
#include <vector>
#include <string>
#include <strsafe.h>
#include <thread>

// defined in lens app
#define VIRTUAL_OUTPUT_RESOLUTION_FILE_NAME L"\\PRISMLens\\virtual-cam\\prism-virtual-cam"

#define LENS_DEFAULT_WIDTH 1280
#define LENS_DEFAULT_HEIGHT 720

struct lens_device_info {
	std::string video_name = "";
	std::string audio_name = "";
	bool is_enabled = true;
	int width = 0;
	int height = 0;
};

std::recursive_mutex lock_lens_info;
lens_device_info lens_devices[MAX_LENS_COUNT] = {
	{TEXT_PRISM_LENS_1, TEXT_PRISM_LENS_AUDIO_1, true, LENS_DEFAULT_WIDTH, LENS_DEFAULT_HEIGHT},
	{TEXT_PRISM_LENS_2, TEXT_PRISM_LENS_AUDIO_2, true, LENS_DEFAULT_WIDTH, LENS_DEFAULT_HEIGHT},
	{TEXT_PRISM_LENS_3, TEXT_PRISM_LENS_AUDIO_3, true, LENS_DEFAULT_WIDTH, LENS_DEFAULT_HEIGHT},
};

void get_output_resolution(uint32_t index)
{
	if (index >= MAX_LENS_COUNT) {
		assert(false);
		return;
	}

	LONG cx = 0, cy = 0, fps = 0;

	std::array<wchar_t, MAX_PATH> res_file = {0};
	std::wstring file_name(VIRTUAL_OUTPUT_RESOLUTION_FILE_NAME);
	SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, res_file.data());
	StringCbCat(res_file.data(), res_file.size(), file_name.append(std::to_wstring(index) + L".txt").c_str());

	HANDLE file = CreateFileW(res_file.data(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (file) {
		char res[128];
		DWORD len = 0;

		if (ReadFile(file, res, sizeof(res) - 1, &len, nullptr)) {
			res[len] = 0;
			int vals = sscanf(res, "%ldx%ldx%ld", &cx, &cy, &fps);
			if (vals == 3 && cx > 0 && cy > 0) {
				std::lock_guard<std::recursive_mutex> lock(lock_lens_info);
				lens_devices[index].width = cx;
				lens_devices[index].height = cy;
			}
		}

		CloseHandle(file);
	}
}

EXPORT void pls_init_lens_resolution()
{
	static DWORD64 pre_init_time = 0;

	DWORD64 cur_time = GetTickCount64();
	if (cur_time > pre_init_time + 2000) { // control the initialization interval
		pre_init_time = cur_time;

		get_output_resolution(0);
		get_output_resolution(1);
		get_output_resolution(2);
	}
}

EXPORT void pls_check_lens_resolution(uint32_t lens_index)
{
	get_output_resolution(lens_index);
}

EXPORT void pls_set_lens_resolution(uint32_t index, int width, int height)
{
	if (index >= MAX_LENS_COUNT || width <= 0 || height <= 0) {
		assert(false);
		return;
	}

	std::lock_guard<std::recursive_mutex> lock(lock_lens_info);
	auto &item = lens_devices[index];
	if (item.width != width || item.height != height) {
		item.width = width;
		item.height = height;
	}
}

EXPORT void pls_get_lens_resolution(uint32_t index, int *width, int *height)
{
	if (!width || !height) {
		assert(false);
		return;
	}

	*width = LENS_DEFAULT_WIDTH;
	*height = LENS_DEFAULT_HEIGHT;

	if (index >= MAX_LENS_COUNT) {
		assert(false);
		return;
	}

	std::lock_guard<std::recursive_mutex> lock(lock_lens_info);
	const auto &item = lens_devices[index];
	if (item.width > 0 && item.height > 0) {
		*width = item.width;
		*height = item.height;
	}
}

EXPORT void pls_enum_lens_devices(pls_enum_lens_device_callback_t callback, void *param)
{
	if (!callback || !param) {
		assert(false);
		return;
	}

	std::lock_guard<std::recursive_mutex> lock(lock_lens_info);
	for (size_t i = 0; i < MAX_LENS_COUNT; i++) {
		const auto &item = lens_devices[i];
		callback(param, i, item.audio_name.c_str(), item.video_name.c_str(), item.is_enabled, item.width,
			 item.height);
	}
}

#endif // _WIN32

#if defined(__APPLE__)
#include <cstdio>
#include <cstring>
#include <mutex>
#include "util/base.h"
#include "util/platform.h"

#define LENS_DEFAULT_WIDTH 1280
#define LENS_DEFAULT_HEIGHT 720

#define LENS_VIRTUAL_CAM_DIR "Library/Application Support/PRISMLens/virtual-cam"
#define LENS_VIRTUAL_CAM_FILE "prism-virtual-cam"

struct lens_resolution {
	int width = LENS_DEFAULT_WIDTH;
	int height = LENS_DEFAULT_HEIGHT;
};

std::recursive_mutex lock_lens_info;
lens_resolution lens_resolutions[MAX_LENS_COUNT];

static void get_output_resolution(uint32_t index)
{
	if (index >= MAX_LENS_COUNT)
		return;

	const char *home = getenv("HOME");
	if (!home || !home[0])
		return;

	char path[1024];
	int n = snprintf(path, sizeof(path), "%s/%s/%s%u.txt", home, LENS_VIRTUAL_CAM_DIR, LENS_VIRTUAL_CAM_FILE,
			 index);
	if (n <= 0 || n >= (int)sizeof(path))
		return;

	FILE *fp = fopen(path, "r");
	if (!fp) {
		blog(LOG_WARNING, "Failed to open lens resolution file: %s", path);
		return;
	}

	char res[128] = {0};
	const size_t max_read = sizeof(res) - 1;
	size_t len = fread(res, 1, max_read, fp);
	const bool io_err = ferror(fp) != 0;
	fclose(fp);
	if (io_err) {
		blog(LOG_WARNING, "Failed to read lens resolution file: %s", path);
		return;
	}
	if (len == max_read) {
		blog(LOG_WARNING, "Lens resolution file line too long or truncated: %s", path);
		return;
	}
	res[len] = '\0';

	long cx = 0, cy = 0, fps = 0;
	int vals = sscanf(res, "%ldx%ldx%ld", &cx, &cy, &fps);
	if (vals == 3 && cx > 0 && cy > 0 && fps > 0) {
		std::lock_guard<std::recursive_mutex> lock(lock_lens_info);
		lens_resolutions[index].width = (int)cx;
		lens_resolutions[index].height = (int)cy;
	} else if (len > 0) {
		blog(LOG_WARNING, "Invalid lens resolution format in file: %s", path);
	}
}

EXPORT void pls_init_lens_resolution()
{
	static uint64_t pre_init_time = 0;

	uint64_t cur_time = os_gettime_ns() / 1000000ULL;
	if (cur_time > pre_init_time + 2000) {
		pre_init_time = cur_time;
		get_output_resolution(0);
		get_output_resolution(1);
		get_output_resolution(2);
	}
}

EXPORT void pls_get_lens_resolution(uint32_t index, int *width, int *height)
{
	if (!width || !height) {
		return;
	}

	*width = LENS_DEFAULT_WIDTH;
	*height = LENS_DEFAULT_HEIGHT;

	if (index >= MAX_LENS_COUNT) {
		return;
	}

	std::lock_guard<std::recursive_mutex> lock(lock_lens_info);
	const auto &item = lens_resolutions[index];
	if (item.width > 0 && item.height > 0) {
		*width = item.width;
		*height = item.height;
	}
}

#endif // __APPLE__
