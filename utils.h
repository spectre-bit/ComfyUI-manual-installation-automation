/*
the header file for utils.c
Copyright (C) 2026  spectre-bit

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef UTILS_H
#define UTILS_H

#include <windows.h>

#include "path_utils.h"
#include "process_utils.h"

#define L_RED L"\x1b[31m"
#define L_GREEN L"\x1b[32m"
#define L_RESET L"\x1b[0m"

#define GREEN "\x1b[32m"
#define YELLOW "\033[33m"

error_code println_fill_width(readonly_wstring text, HANDLE h_console);

#define println_success_fill(text, h_console) {error_code println_success_fill_error_code = println_fill_width((readonly_wstring){L_GREEN text L_RESET, ws_len(L_GREEN text L_RESET)}, h_console); if (println_success_fill_error_code != SUCCESS) {return println_success_fill_error_code;}}
#define println_error_fill(text, h_console) {error_code println_error_fill_error_code = println_fill_width((readonly_wstring){L_RED text L_RESET , ws_len(L_RED text L_RESET)}, h_console); if (println_error_fill_error_code != SUCCESS) {return println_error_fill_error_code;}}

error_code get_latest_tag(tool_info *t_info, process *p, readonly_wstring *tag_out, HANDLE h_out);

error_code wait_with_animation(HANDLE h_job, HANDLE h_process, bool is_console, HANDLE h_out, readonly_wstring text, readonly_wstring success_text, readonly_wstring failure_text);

error_code install_git(bool is_console, HANDLE h_out, readonly_wstring text);

error_code install_python(bool is_console, HANDLE h_out, readonly_wstring text, version v);

error_code init_console(HANDLE *h_out, bool *is_console);

error_code get_own_path(readonly_wstring *out);

error_code get_ini_string_arg(wchar_t *path, wchar_t *key, readonly_wstring *out);

error_code to_absolute_path(wchar_t *path, readonly_wstring *out);

#endif //UTILS_H