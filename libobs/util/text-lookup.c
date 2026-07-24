/*
 * Copyright (c) 2023 Lain Bailey <lain@obsproject.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <ctype.h>

#include "dstr.h"
#include "text-lookup.h"
#include "lexer.h"
#include "platform.h"
#include "uthash.h"
#include "pls/pls-obs-api.h"

/* ------------------------------------------------------------------------- */

struct text_item {
	char *lookup, *value;
	UT_hash_handle hh;
};

static inline void text_item_destroy(struct text_item *item)
{
	bfree(item->lookup);
	bfree(item->value);
	bfree(item);
}

/* ------------------------------------------------------------------------- */

struct text_lookup {
	struct text_item *items;
	struct text_item *english_items;
	struct text_item *other_language_items;
};

static void lookup_getstringtoken(struct lexer *lex, struct strref *token)
{
	const char *temp = lex->offset;
	bool was_backslash = false;

	while (*temp != 0 && *temp != '\n') {
		if (!was_backslash) {
			if (*temp == '\\') {
				was_backslash = true;
			} else if (*temp == '"') {
				temp++;
				break;
			}
		} else {
			was_backslash = false;
		}

		++temp;
	}

	token->len += (size_t)(temp - lex->offset);

	if (*token->array == '"') {
		token->array++;
		token->len--;

		if (*(temp - 1) == '"')
			token->len--;
	}

	lex->offset = temp;
}

static bool lookup_gettoken(struct lexer *lex, struct strref *str)
{
	struct base_token temp;

	base_token_clear(&temp);
	strref_clear(str);

	while (lexer_getbasetoken(lex, &temp, PARSE_WHITESPACE)) {
		char ch = *temp.text.array;

		if (!str->array) {
			/* comments are designated with a #, and end at LF */
			if (ch == '#') {
				while (*lex->offset != '\n' && *lex->offset != 0)
					++lex->offset;
			} else if (temp.type == BASETOKEN_WHITESPACE) {
				strref_copy(str, &temp.text);
				break;
			} else {
				strref_copy(str, &temp.text);
				if (ch == '"') {
					lookup_getstringtoken(lex, str);
					break;
				} else if (ch == '=') {
					break;
				}
			}
		} else {
			if (temp.type == BASETOKEN_WHITESPACE || *temp.text.array == '=') {
				lex->offset -= temp.text.len;
				break;
			}

			if (ch == '#') {
				lex->offset--;
				break;
			}

			str->len += temp.text.len;
		}
	}

	return (str->len != 0);
}

static inline bool lookup_goto_nextline(struct lexer *p)
{
	struct strref val;
	bool success = true;

	strref_clear(&val);

	while (true) {
		if (!lookup_gettoken(p, &val)) {
			success = false;
			break;
		}
		if (*val.array == '\n')
			break;
	}

	return success;
}

static char *convert_string(const char *str, size_t len)
{
	struct dstr out;
	out.array = bstrdup_n(str, len);
	out.capacity = len + 1;
	out.len = len;

	dstr_replace(&out, "\\n", "\n");
	dstr_replace(&out, "\\t", "\t");
	dstr_replace(&out, "\\r", "\r");
	dstr_replace(&out, "\\\"", "\"");

	// PRISM/Liuying/20230516/#724/replace obs to prism
	dstr_replace(&out, "OBS Studio", "PRISM Live Studio");
	dstr_replace(&out, "OBS", "PRISM");

	return out.array;
}

//PRISM/aiguanghua/20241203/PRISM_PC-1698/save the en-US.ini text item
static void lookup_addfiledata(struct text_lookup *lookup, const char *file_data, bool isEnglish)
{
	struct lexer lex;
	struct strref name, value;

	lexer_init(&lex);
	lexer_start(&lex, file_data);
	strref_clear(&name);
	strref_clear(&value);

	while (lookup_gettoken(&lex, &name)) {
		struct text_item *item;
		struct text_item *old;
		bool got_eq = false;

		if (*name.array == '\n')
			continue;
	getval:
		if (!lookup_gettoken(&lex, &value))
			break;
		if (*value.array == '\n')
			continue;
		else if (!got_eq && *value.array == '=') {
			got_eq = true;
			goto getval;
		}

		item = bzalloc(sizeof(struct text_item));
		item->lookup = bstrdup_n(name.array, name.len);
		item->value = convert_string(value.array, value.len);

		//PRISM/wangshaohui/20231122/none/compatible using incorrect key
		size_t len = strlen(item->lookup);
		for (size_t i = 0; i < len; i++) {
			item->lookup[i] = tolower(item->lookup[i]);
		}

		//PRISM/aiguanghua/20241203/PRISM_PC-1698/save the en-US.ini text item
		if (isEnglish) {
			struct text_item *english_item = bzalloc(sizeof(struct text_item));
			english_item->lookup = bstrdup_n(item->lookup, strlen(item->lookup));
			english_item->value = bstrdup_n(item->value, strlen(item->value));
			HASH_REPLACE_STR(lookup->english_items, lookup, english_item, old);
			if (old)
				text_item_destroy(old);
		} else {
			struct text_item *other_language_item = bzalloc(sizeof(struct text_item));
			other_language_item->lookup = bstrdup_n(item->value, strlen(item->value));
			other_language_item->value = bstrdup_n(item->lookup, strlen(item->lookup));
			HASH_REPLACE_STR(lookup->other_language_items, lookup, other_language_item, old);
			if (old)
				text_item_destroy(old);
		}

		HASH_REPLACE_STR(lookup->items, lookup, item, old);

		if (old)
			text_item_destroy(old);

		if (!lookup_goto_nextline(&lex))
			break;
	}

	lexer_free(&lex);
}

/*
//PRISM/wangshaohui/20231122/none/compatible using incorrect key 
static inline bool lookup_getstring(const char *lookup_val, const char **out, struct text_lookup *lookup)
{
	struct text_item *item;

	if (!lookup->items)
		return false;

	HASH_FIND_STR(lookup->items, lookup_val, item);

	if (!item)
		return false;

	*out = item->value;
	return true;
}
*/

//PRISM/wangshaohui/20231122/none/compatible using incorrect key
#define MAX_KEY_LEN 256
static inline bool lookup_getstring(const char *lookup_val, const char **out, struct text_lookup *lookup)
{
	struct text_item *item;

	if (!lookup->items || !lookup_val || !out)
		return false;

	size_t len = strlen(lookup_val);
	if (len >= MAX_KEY_LEN) {
		assert(false && "key too long");
		return false;
	}

	char key[MAX_KEY_LEN]; // we'd better avoid using heap memory every time
	for (size_t i = 0; i < len; i++) {
		key[i] = (char)tolower(lookup_val[i]);
	}
	key[len] = 0;

	HASH_FIND_STR(lookup->items, key, item);

	if (!item)
		return false;

	*out = item->value;
	return true;
}

/* ------------------------------------------------------------------------- */

lookup_t *text_lookup_create(const char *path)
{
	struct text_lookup *lookup = bzalloc(sizeof(struct text_lookup));
	
	if (!text_lookup_add(lookup, path)) {
		bfree(lookup);
		lookup = NULL;
	}

	//PRISM/aiguanghua/20241203/PRISM_PC-1698/save the en-US.ini text item
	pls_add_text_lookup(lookup);

	return lookup;
}

//PRISM/aiguanghua/20241203/PRISM_PC-1698/save the en-US.ini text item
bool ends_with(const char *str, const char *suffix)
{
	if (!str || !suffix) {
		return false;
	}

	size_t str_len = strlen(str);
	size_t suffix_len = strlen(suffix);

	if (str_len < suffix_len) {
		return false;
	}

	return strcmp(str + str_len - suffix_len, suffix) == 0;
}

bool text_lookup_add(lookup_t *lookup, const char *path)
{
	struct dstr file_str;
	char *temp = NULL;
	FILE *file;

	file = os_fopen(path, "rb");
	if (!file)
		return false;

	os_fread_utf8(file, &temp);
	dstr_init_move_array(&file_str, temp);
	fclose(file);

	if (!file_str.array)
		return false;

	dstr_replace(&file_str, "\r", " ");

	//PRISM/aiguanghua/20241203/PRISM_PC-1698/save the en-US.ini text item
	bool isEn = false;
	if (ends_with(path, "en-US.ini")) {
		isEn = true;
	}
	lookup_addfiledata(lookup, file_str.array, isEn);
	dstr_free(&file_str);

	return true;
}

void text_lookup_destroy(lookup_t *lookup)
{
	if (lookup) {

		struct text_item *item, *tmp;
		HASH_ITER (hh, lookup->items, item, tmp) {
			HASH_DELETE(hh, lookup->items, item);
			text_item_destroy(item);
		}

		struct text_item *english_item;
		HASH_ITER (hh, lookup->english_items, english_item, tmp) {
			HASH_DELETE(hh, lookup->english_items, english_item);
			text_item_destroy(english_item);
		}

		struct text_item *other_language_item;
		HASH_ITER (hh, lookup->other_language_items, other_language_item, tmp) {
			HASH_DELETE(hh, lookup->other_language_items, other_language_item);
			text_item_destroy(other_language_item);
		}

		//PRISM/aiguanghua/20241203/PRISM_PC-1698/save the en-US.ini text item
		pls_remove_module_lookup_map_by_value(lookup);
		pls_remove_text_lookup(lookup);

		bfree(lookup);
	}
}

bool text_lookup_getstr(lookup_t *lookup, const char *lookup_val, const char **out)
{
	if (lookup)
		return lookup_getstring(lookup_val, out, lookup);
	return false;
}

//PRISM/aiguanghua/20241203/PRISM_PC-1698/save the en-US.ini text item
bool text_lookup_get_plugin_english_str(const char *kr_str, const char *plugin_id, const char **out)
{

	if (!kr_str || !plugin_id || !out)
		return false;

	lookup_t *lookup = pls_get_lookup_by_id(plugin_id);
	if (!lookup) {
		return false;
	}

	struct text_item *keyItem;
	HASH_FIND_STR(lookup->other_language_items, kr_str, keyItem);
	if (!keyItem) {
		return false;
	}

	struct text_item *enItem;
	HASH_FIND_STR(lookup->english_items, keyItem->value, enItem);
	if (!enItem) {
		return false;
	}

	*out = enItem->value;
	return true;
}

//PRISM/aiguanghua/20241203/PRISM_PC-1698/save the en-US.ini text item
EXPORT bool text_lookup_get_english_str(lookup_t *lookup, const char *kr_str, const char **out)
{
	if (!lookup || !kr_str || !out)
		return false;

	if (!pls_is_valid_text_lookup(lookup)) {
		return false;
	}

	struct text_item *keyItem;
	HASH_FIND_STR(lookup->other_language_items, kr_str, keyItem);
	if (!keyItem) {
		return false;
	}

	struct text_item *enItem;
	HASH_FIND_STR(lookup->english_items, keyItem->value, enItem);
	if (!enItem) {
		return false;
	}

	*out = enItem->value;
	return true;
}

//PRISM/aiguanghua/20241203/PRISM_PC-1698/save the en-US.ini text item
EXPORT bool text_lookup_get_english_str_by_key(lookup_t *lookup, const char *key, const char **out)
{
	if (!lookup || !key || !out)
		return false;

	if (!pls_is_valid_text_lookup(lookup)) {
		return false;
	}

	size_t len = strlen(key);
	if (len >= MAX_KEY_LEN) {
		assert(false && "key too long");
		return false;
	}

	char lower_key[MAX_KEY_LEN]; // we'd better avoid using heap memory every time
	for (size_t i = 0; i < len; i++) {
		lower_key[i] = (char)tolower(key[i]);
	}
	lower_key[len] = 0;

	struct text_item *enItem;
	HASH_FIND_STR(lookup->english_items, lower_key, enItem);
	if (!enItem) {
		return false;
	}

	*out = enItem->value;
	return true;
}

//PRISM/aiguanghua/20241203/PRISM_PC-1698/save the en-US.ini text item
EXPORT bool getstring_pointer_get_english_str_by_key(void *getstring_pointer, const char *key, const char **out)
{
	if (!getstring_pointer || !key || !out)
		return false;

	lookup_t *lookup = pls_get_lookup_by_getstring_pointer(getstring_pointer);
	if (!lookup) {
		return false;
	}

	return text_lookup_get_english_str_by_key(lookup, key, out);
}

//PRISM/aiguanghua/20241203/PRISM_PC-1698/save the en-US.ini text item
EXPORT bool getstring_pointer_get_english_str(void *getstring_pointer, const char *kr_str, const char **out)
{
	if (!getstring_pointer || !kr_str || !out)
		return false;

	lookup_t *lookup = pls_get_lookup_by_getstring_pointer(getstring_pointer);
	if (!lookup) {
		return false;
	}

	return text_lookup_get_english_str(lookup, kr_str, out);
}
