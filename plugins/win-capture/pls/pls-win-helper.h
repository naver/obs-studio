#pragma once
#include <Windows.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

void pls_game_failed_push(HWND target);
void pls_game_failed_remove(HWND target);
void pls_game_failed_clear();
bool pls_game_failed_exist(HWND target);

#ifdef __cplusplus
}
#endif
