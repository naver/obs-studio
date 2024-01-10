#include "pls-properties.h"
#include <stdint.h>
#include <map>
#include <mutex>

#include "util/bmem.h"

using namespace std;

struct pls_property {
	uint32_t flags;
	char *placeholder;
	bool ignoreCallbackWhenRefresh;
	int length_limit;
};

static mutex g_mutexPRISMProperty;
static map<obs_property *, pls_property> g_mapPRISMProperty;

extern "C" {
void pls_property_set_flags(obs_property_t *p, uint32_t flags)
{
	lock_guard _(g_mutexPRISMProperty);

	auto &prop = g_mapPRISMProperty[p];
	prop.flags = flags;
}

uint32_t pls_property_get_flags(obs_property_t *p)
{
	lock_guard _(g_mutexPRISMProperty);

	const auto &prop = g_mapPRISMProperty[p];
	return prop.flags;
}

void pls_property_set_placeholder(obs_property_t *p, const char *placeholder)
{
	lock_guard _(g_mutexPRISMProperty);

	auto &prop = g_mapPRISMProperty[p];
	prop.placeholder = bstrdup(placeholder);
}

const char *pls_property_get_placeholder(obs_property_t *p)
{
	lock_guard _(g_mutexPRISMProperty);

	const auto &prop = g_mapPRISMProperty[p];
	return prop.placeholder;
}

void pls_property_set_ignore_callback_when_refresh(obs_property_t *p,
						   bool ignored)
{
	lock_guard _(g_mutexPRISMProperty);

	auto &prop = g_mapPRISMProperty[p];
	prop.ignoreCallbackWhenRefresh = ignored;
}

bool pls_property_get_ignore_callback_when_refresh(obs_property_t *p)
{
	lock_guard _(g_mutexPRISMProperty);

	const auto &prop = g_mapPRISMProperty[p];
	return prop.ignoreCallbackWhenRefresh;
}

void pls_property_set_length_limit(obs_property_t *p, int max_length)
{
	lock_guard _(g_mutexPRISMProperty);

	auto &prop = g_mapPRISMProperty[p];
	prop.length_limit = max_length;
}

int pls_property_get_length_limit(obs_property_t *p)
{
	lock_guard _(g_mutexPRISMProperty);

	const auto &prop = g_mapPRISMProperty[p];
	return prop.length_limit;
}

void pls_property_map_erase(obs_property_t *p)
{
	lock_guard _(g_mutexPRISMProperty);

	auto iter = g_mapPRISMProperty.find(p);
	if (iter != g_mapPRISMProperty.end()) {
		bfree(iter->second.placeholder);

		g_mapPRISMProperty.erase(iter);
	}
}
}
