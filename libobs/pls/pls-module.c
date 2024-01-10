#include "pls-obs-api.h"

#include "../obs-module.c"

//PRISM/Zhangdewen/20230117/#/load with filter
struct pls_load_all_callback_info {
	struct fail_info fi;
	bool has_mfi;
	pls_load_module_filter_t filter;
};

//PRISM/Zhangdewen/20230117/#/load with filter
static void pls_load_all_callback(void *param,
				  const struct obs_module_info2 *info)
{
	struct pls_load_all_callback_info *laci = param;
	if (laci->filter && !laci->filter(info->bin_path)) {
		blog(LOG_INFO, "Should ignore loading module file '%s'",
		     info->bin_path);
		return;
	}

	if (laci->has_mfi) {
		load_all_callback(&laci->fi, info);
	} else {
		load_all_callback(NULL, info);
	}
}

//PRISM/Zhangdewen/20230117/#/load with filter
static const char *pls_load_all_modules_name = "pls_load_all_modules";

//PRISM/Zhangdewen/20230117/#/load with filter
void pls_load_all_modules(pls_load_module_filter_t filter)
{
	struct pls_load_all_callback_info laci;
	memset(&laci, 0, sizeof(laci));
	laci.has_mfi = false;
	laci.filter = filter;

	profile_start(pls_load_all_modules_name);
	obs_find_modules2(pls_load_all_callback, &laci);
#ifdef _WIN32
	profile_start(reset_win32_symbol_paths_name);
	reset_win32_symbol_paths();
	profile_end(reset_win32_symbol_paths_name);
#endif
	profile_end(pls_load_all_modules_name);
}

//PRISM/Zhangdewen/20230117/#/load with filter
static const char *pls_load_all_modules2_name = "pls_load_all_modules2";

//PRISM/Zhangdewen/20230117/#/load with filter
void pls_load_all_modules2(struct obs_module_failure_info *mfi,
			   pls_load_module_filter_t filter)
{
	struct pls_load_all_callback_info laci;
	memset(&laci, 0, sizeof(laci));
	laci.has_mfi = true;
	laci.filter = filter;

	memset(mfi, 0, sizeof(*mfi));

	profile_start(pls_load_all_modules2_name);
	obs_find_modules2(pls_load_all_callback, &laci);
#ifdef _WIN32
	profile_start(reset_win32_symbol_paths_name);
	reset_win32_symbol_paths();
	profile_end(reset_win32_symbol_paths_name);
#endif
	profile_end(pls_load_all_modules2_name);

	mfi->count = laci.fi.fail_count;
	mfi->failed_modules =
		strlist_split(laci.fi.fail_modules.array, ';', false);
	dstr_free(&laci.fi.fail_modules);
}
