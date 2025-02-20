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

#ifdef _WIN32
#include <windows.h>
static bool has_pls_export(VOID *base, PIMAGE_NT_HEADERS nt_headers)
{
	__try {
		PIMAGE_DATA_DIRECTORY data_dir;
		data_dir =
			&nt_headers->OptionalHeader
				 .DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];

		if (data_dir->Size == 0)
			return false;

		PIMAGE_SECTION_HEADER section, last_section;
		section = IMAGE_FIRST_SECTION(nt_headers);
		last_section = section;

		/* find the section that contains the export directory */
		int i;
		for (i = 0; i < nt_headers->FileHeader.NumberOfSections; i++) {
			if (section->VirtualAddress <=
			    data_dir->VirtualAddress) {
				last_section = section;
				section++;
				continue;
			} else {
				break;
			}
		}

		/* double check in case we exited early */
		if (last_section->VirtualAddress > data_dir->VirtualAddress ||
		    section->VirtualAddress <= data_dir->VirtualAddress)
			return false;

		section = last_section;

		/* get a pointer to the export directory */
		PIMAGE_EXPORT_DIRECTORY export;
		export = (PIMAGE_EXPORT_DIRECTORY)((byte *)base +
						   data_dir->VirtualAddress -
						   section->VirtualAddress +
						   section->PointerToRawData);

		if (export->NumberOfNames == 0)
			return false;

		/* get a pointer to the export directory names */
		DWORD *names_ptr;
		names_ptr = (DWORD *)((byte *)base + export->AddressOfNames -
				      section->VirtualAddress +
				      section->PointerToRawData);

		/* iterate through each name and see if its an obs plugin */
		CHAR *name;
		size_t j;
		for (j = 0; j < export->NumberOfNames; j++) {

			name = (CHAR *)base + names_ptr[j] -
			       section->VirtualAddress +
			       section->PointerToRawData;

			if (!strcmp(name, "obs_is_internal_module")) {
				return true;
			}
		}
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		/* we failed somehow, for compatibility let's assume it
		 * was a valid plugin and let the loader deal with it */
		return true;
	}

	return false;
}

void pls_get_plugin_info(const char *path, bool *is_pls_plugin)
{
	struct dstr dll_name;
	wchar_t *wpath;

	HANDLE hFile = INVALID_HANDLE_VALUE;
	HANDLE hFileMapping = NULL;
	VOID *base = NULL;

	PIMAGE_DOS_HEADER dos_header;
	PIMAGE_NT_HEADERS nt_headers;

	*is_pls_plugin = false;

	if (!path)
		return;

	dstr_init_copy(&dll_name, path);
	dstr_replace(&dll_name, "\\", "/");
	if (!dstr_find(&dll_name, ".dll"))
		dstr_cat(&dll_name, ".dll");
	os_utf8_to_wcs_ptr(dll_name.array, 0, &wpath);

	dstr_free(&dll_name);

	hFile = CreateFileW(wpath, GENERIC_READ, FILE_SHARE_READ, NULL,
			    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	bfree(wpath);

	if (hFile == INVALID_HANDLE_VALUE)
		goto cleanup;

	hFileMapping =
		CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (hFileMapping == NULL)
		goto cleanup;

	base = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
	if (!base)
		goto cleanup;

	/* all mapped file i/o must be prepared to handle exceptions */
	__try {

		dos_header = (PIMAGE_DOS_HEADER)base;

		if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
			goto cleanup;

		nt_headers = (PIMAGE_NT_HEADERS)((byte *)dos_header +
						 dos_header->e_lfanew);

		if (nt_headers->Signature != IMAGE_NT_SIGNATURE)
			goto cleanup;

		*is_pls_plugin = has_pls_export(base, nt_headers);
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		/* we failed somehow, for compatibility let's assume it
		 * was a valid plugin and let the loader deal with it */
		*is_pls_plugin = true;
		goto cleanup;
	}

cleanup:
	if (base)
		UnmapViewOfFile(base);

	if (hFileMapping != NULL)
		CloseHandle(hFileMapping);

	if (hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
}

bool os_is_pls_plugin(const char *path)
{
	bool is_pls_plugin;

	pls_get_plugin_info(path, &is_pls_plugin);

	return is_pls_plugin;
}
#endif
