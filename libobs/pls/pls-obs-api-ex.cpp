#include "pls-obs-api.h"
#include <set>
#include <string>
#include <algorithm>

std::set<std::string> g_loaded_dll;

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
