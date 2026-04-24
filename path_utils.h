/*
the header file for path_utils.c
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

#ifndef CPPTEST_PATH_UTILS_H
#define CPPTEST_PATH_UTILS_H

#define _CRT_SECURE_NO_WARNINGS

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define ws_size(str) (sizeof(str) / sizeof(wchar_t))
#define ws_len(str) ((sizeof(str) / sizeof(wchar_t)) - 1)

#define len_to_size(len) (len + 1)
#define size_to_len(size) (size - 1)

#define s_len(str) (sizeof(str) - 1)
#define s_size(str) (sizeof(str))

typedef enum error_code : uint8_t {
    SUCCESS,
    WINAPI_FUNCTION_ERROR,
    MALLOC_FAILURE,
    REALLOC_FAILURE,
    INVALID_ARGUMENT,
    PATH_NOT_FOUND,
    VERSION_MISMATCH,
    NULL_POINTER_ERROR,
    IS_NO_CONSOLE,
    INVALID_HANDLE,
    ERROR_FETCHING_TAG,
    TOO_MANY_ARGUMENTS,
    FAILURE,
    ELEVATION_DENIED
} error_code;

typedef struct readonly_wstring {
    wchar_t *buffer;  // contains \0
    size_t len;  // strlen
} readonly_wstring;

typedef struct version {
    union {
        struct {
            int major;
            int minor;
            int patch;
            int build;
        };
        int components[4];
    };
    unsigned int how_many;
} version;

typedef struct tool_info {
    readonly_wstring path;
    readonly_wstring tool_name;
    readonly_wstring sub_key;
    readonly_wstring value;
    readonly_wstring folder;
    version version;
    bool use_version;
    bool has_long_path;
} tool_info;

error_code wstr_cat(size_t result_buffer_len, readonly_wstring *result, int count, ...);

#define RED "\x1b[31m"
#define RESET "\x1b[0m"

#define println_error(format, ...) fprintf(stderr, RED format RESET "\n" __VA_OPT__(,) __VA_ARGS__)
#define null_check(ptr, func_name) {if (!ptr) {println_error(func_name ": null pointer error: " #ptr " is nullptr"); return NULL_POINTER_ERROR;}}

error_code init_tool_info(tool_info *t_info);

#define t_set_name(t_info, name) (t_info)->tool_name = (readonly_wstring){name, ws_len(name)}
#define t_set_sub_key(t_info, subKey) (t_info)->sub_key = (readonly_wstring){subKey, ws_len(subKey)}
#define t_set_value(t_info, Value) (t_info)->value = (readonly_wstring){Value, ws_len(Value)}
#define t_set_folder(t_info, Folder) (t_info)->folder = (readonly_wstring){Folder, ws_len(Folder)}

error_code cmp_version(version v1, version v2, int *out);

error_code get_exe_string(const tool_info *t_info, readonly_wstring *out);

error_code get_version_string(version v, readonly_wstring *out);

error_code sub_key_resolve_version(const tool_info *t_info, readonly_wstring *out);

error_code resolve_long_path(const tool_info *t_info, readonly_wstring *out);

error_code get_exe_version(const tool_info *t_info, version *out);

error_code path_combine(readonly_wstring path1, readonly_wstring path2, readonly_wstring *out);

error_code search_path(tool_info *t_info);

error_code search_app_paths(tool_info *t_info);

error_code search_uninstall_paths(tool_info *t_info);

error_code search_sub_key(tool_info *t_info);

error_code search_tool(tool_info *t_info);

#endif //CPPTEST_PATH_UTILS_H