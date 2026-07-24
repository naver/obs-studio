#include "pls-win-helper.h"
#include <algorithm>
#include <vector>
#include <mutex>

struct game_failed_target_key {
	HWND hwnd = nullptr;
	DWORD process_id = 0;

	bool operator==(const game_failed_target_key &other) const
	{
		return hwnd == other.hwnd && process_id == other.process_id;
	}
};

static game_failed_target_key make_game_failed_target_key(HWND target)
{
	game_failed_target_key key;
	key.hwnd = target;
	GetWindowThreadProcessId(target, &key.process_id);
	return key;
}

struct game_failed_array_helper {
	std::vector<game_failed_target_key> array_data;
	std::recursive_mutex array_mutex;
} g_gameFailedWnd;

void pls_game_failed_push(HWND target)
{
	if (!target || !IsWindow(target))
		return;

	const auto key = make_game_failed_target_key(target);
	std::lock_guard<std::recursive_mutex> lock(g_gameFailedWnd.array_mutex);
	if (std::find(g_gameFailedWnd.array_data.begin(), g_gameFailedWnd.array_data.end(), key) !=
	    g_gameFailedWnd.array_data.end())
		return;

	g_gameFailedWnd.array_data.push_back(key);
}

void pls_game_failed_remove(HWND target)
{
	const auto key = make_game_failed_target_key(target);
	std::lock_guard<std::recursive_mutex> lock(g_gameFailedWnd.array_mutex);
	auto &array_data = g_gameFailedWnd.array_data;
	array_data.erase(std::remove(array_data.begin(), array_data.end(), key), array_data.end());
}

void pls_game_failed_clear()
{
	std::lock_guard<std::recursive_mutex> lock(g_gameFailedWnd.array_mutex);
	g_gameFailedWnd.array_data.clear();
}

bool pls_game_failed_exist(HWND target)
{
	if (!target || !IsWindow(target))
		return false;

	const auto key = make_game_failed_target_key(target);
	std::lock_guard<std::recursive_mutex> lock(g_gameFailedWnd.array_mutex);
	return std::find(g_gameFailedWnd.array_data.begin(), g_gameFailedWnd.array_data.end(), key) !=
	       g_gameFailedWnd.array_data.end();
}
