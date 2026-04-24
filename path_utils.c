/*
some utility to get a path for installed program, from places in the registry or the path
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

#define _CRT_SECURE_NO_WARNINGS

#ifdef _DEBUG
    #define _CRTDBG_MAP_ALLOC
    #include <crtdbg.h>
#endif

#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <pathcch.h>
#include <shlwapi.h>

#include "path_utils.h"

#define LONG_PATH_PREFIX L"\\\\?\\"

error_code wstr_cat(size_t result_buffer_len, readonly_wstring *result, int count, ...) {
    wchar_t *buffer = malloc((len_to_size(result_buffer_len)) * sizeof(wchar_t));
    if (!buffer) {
        println_error("wstr_cat: malloc failure: %s", strerror(errno));
        return MALLOC_FAILURE;
    }

    va_list args;
    va_start(args, count);

    size_t offset = 0;

    for (int i = 0; i < count; ++i) {
        readonly_wstring arg = va_arg(args, readonly_wstring);

        memcpy(buffer + offset, arg.buffer, arg.len * sizeof(wchar_t));
        offset += arg.len;
    }
    va_end(args);

    buffer[offset] = L'\0';

    result->buffer = buffer;
    result->len = result_buffer_len;

    return SUCCESS;
}

error_code init_tool_info(tool_info *t_info) {
    null_check(t_info, "init_tool_info");
    *t_info = (tool_info){};
    return SUCCESS;
}

error_code cmp_version(const version v1, const version v2, int *out) {
    null_check(out, "cmp_version");

    if (v1.how_many < 1 || v1.how_many > 4) {
        println_error("cmp_version: invalid argument: const version v1: how_many member contains an invalid value: %d\nhow_many has to be in [1,3]\n", v1.how_many);
        return INVALID_ARGUMENT;
    }
    if (v2.how_many < v1.how_many) {
        println_error("cmp_version: invalid argument: const version v2: how_many member needs to be at least as big as v1.how_many\nv1.how_many: %d\nv2.how_many: %d\n", v1.how_many, v2.how_many);
        return INVALID_ARGUMENT;
    }

    for (int i = 0; i < v1.how_many; ++i) {
        if (v1.components[i] != v2.components[i]) {
            *out = v1.components[i] < v2.components[i] ? -1 : 1;
            return SUCCESS;
        }
    }

    *out = 0;
    return SUCCESS;
}

error_code get_exe_string(const tool_info *t_info, readonly_wstring *out) {
    null_check(t_info, "get_exe_string");
    null_check(t_info->tool_name.buffer, "get_exe_string");
    null_check(out, "get_exe_string");

    constexpr wchar_t exe[] = L".exe";
    out->len = t_info->tool_name.len + ws_len(exe);
    out->buffer = malloc(len_to_size(out->len) * sizeof(wchar_t));
    if (!out->buffer) {
        println_error("get_exe_string: malloc failure: %s", strerror(errno));
        return MALLOC_FAILURE;
    }

    memcpy(out->buffer, t_info->tool_name.buffer, t_info->tool_name.len * sizeof(wchar_t));
    memcpy(out->buffer + t_info->tool_name.len, exe, ws_size(exe) * sizeof(wchar_t));
    return SUCCESS;
}

static int digit_count(int num) {
    if (num == 0) {return 1;}
    num = num < 0 ? -num : num;
    int count = 0;
    while (num > 0) {
        num /= 10;
        ++count;
    }
    return count;
}

static int int_pow(int base, int exponent) {
    if (exponent == 0) {return 1;}
    int result = base;
    for (int i  = 0; i < exponent - 1; ++i) {
        result *= base;
    }
    return result;
}

error_code get_version_string(version v, readonly_wstring *out) {
    null_check(out, "get_version_string");

    if (v.how_many < 1 || v.how_many > 4) {
        println_error("get_version_string: invalid argument for how_many: %d\nhow_many has to be in [1,3]\n", v.how_many);
        return INVALID_ARGUMENT;
    }

    size_t len = 10;
    wchar_t *buffer = malloc(len * sizeof(wchar_t));
    if (!buffer) {
        println_error("get_version_string: malloc failure: %s", strerror(errno));
        return MALLOC_FAILURE;
    }

    int x = 0;
    for (int i = 0; i < v.how_many; ++i) {
        int digits = digit_count(v.components[i]);
        int power = digits - 1;
        for (int j = 0; j < digits; ++j) {
            int divisor = int_pow(10, power--);
            unsigned char digit = v.components[i] / divisor;
            v.components[i] %= divisor;
            buffer[x++] = digit + L'0';

            if (x == len) {
                len *= 2;
                wchar_t *temp = realloc(buffer, len * sizeof(wchar_t));
                if (!temp) {
                    goto realloc_failure;
                }
                buffer = temp;
            }
        }
        if (i < v.how_many - 1) {
            buffer[x++] = L'.';
            if (x == len) {
                len *= 2;
                wchar_t *temp = realloc(buffer, len * sizeof(wchar_t));
                if (!temp) {
                    goto realloc_failure;
                }
                buffer = temp;
            }
        }
    }
    buffer[x] = L'\0';
    len = x;
    wchar_t *temp = realloc(buffer, (len + 1) * sizeof(wchar_t));
    if (!temp) {
        goto realloc_failure;
    }
    *out = (readonly_wstring){temp, len};

    return SUCCESS;

    realloc_failure:
    free(buffer);
    println_error("get_version_string: realloc failure: %s", strerror(errno));
    return REALLOC_FAILURE;
}

static int count_string_specifier(const wchar_t *string) {
    int count = 0;
    while ((string = wcsstr(string, L"%s")) != nullptr) {
        string += ws_len(L"%s");
        ++count;
    }

    return count;
}

error_code sub_key_resolve_version(const tool_info *t_info, readonly_wstring *out) {
    null_check(t_info, "sub_key_resolve_version");
    null_check(t_info->sub_key.buffer, "sub_key_resolve_version");
    null_check(out, "sub_key_resolve_version");

    int count = count_string_specifier(t_info->sub_key.buffer);

    if (count == 0) {
        out->buffer = malloc(len_to_size(t_info->sub_key.len) * sizeof(wchar_t));
        if (!out->buffer) {
            println_error("sub_key_resolve_version: malloc failure: %s", strerror(errno));
            return MALLOC_FAILURE;
        }
        memcpy(out->buffer, t_info->sub_key.buffer, len_to_size(t_info->sub_key.len) * sizeof(wchar_t));
        out->len = t_info->sub_key.len;
        return SUCCESS;
    }

    readonly_wstring version;
    error_code ec;
    if ((ec = get_version_string(t_info->version, &version)) != SUCCESS) {
        return ec;
    }

    size_t len = count * version.len + (t_info->sub_key.len - 2 * count);

    wchar_t *buffer = malloc(len_to_size(len) * sizeof(wchar_t));
    if (!buffer) {
        println_error("resolve_version: malloc failure: %s", strerror(errno));
        return MALLOC_FAILURE;
    }

    size_t buffer_offset = 0;
    size_t sub_key_offset = 0;
    wchar_t *specifier = t_info->sub_key.buffer;
    for (int i = 0; i < count; ++i) {
        specifier = wcsstr(specifier, L"%s");
        ptrdiff_t part_len = specifier - (t_info->sub_key.buffer + sub_key_offset);
        specifier += ws_len(L"%s");
        memcpy(buffer + buffer_offset, t_info->sub_key.buffer + sub_key_offset, part_len * sizeof(wchar_t));
        buffer_offset += part_len;
        sub_key_offset += part_len + ws_len(L"%s");
        memcpy(buffer + buffer_offset, version.buffer, version.len * sizeof(wchar_t));
        buffer_offset += version.len;
    }
    memcpy(buffer + buffer_offset, t_info->sub_key.buffer + sub_key_offset, (t_info->sub_key.len - sub_key_offset + 1) * sizeof(wchar_t));
    buffer[len] = L'\0';

    *out = (readonly_wstring){buffer, len};
    free(version.buffer);

    return SUCCESS;
}

error_code resolve_long_path(const tool_info *t_info, readonly_wstring *out) {
    null_check(t_info, "resolve_long_path");
    null_check(t_info->path.buffer, "resolve_long_path");
    null_check(out, "resolve_long_path");

    if (!t_info->has_long_path) {
        out->buffer = malloc(len_to_size(t_info->path.len) * sizeof(wchar_t));
        if (!out->buffer) {
            println_error("resolve_long_path: malloc failure: %s", strerror(errno));
            return MALLOC_FAILURE;
        }
        memcpy(out->buffer, t_info->path.buffer, len_to_size(t_info->path.len) * sizeof(wchar_t));
        out->len = t_info->path.len;
        return SUCCESS;
    }
    readonly_wstring long_path;
    long_path.len = ws_len(LONG_PATH_PREFIX) + t_info->path.len;
    long_path.buffer = malloc(len_to_size(long_path.len) * sizeof(wchar_t));
    if (!long_path.buffer) {
        println_error("resolve_long_path: malloc failure: %s", strerror(errno));
        return MALLOC_FAILURE;
    }
    memcpy(long_path.buffer, LONG_PATH_PREFIX, ws_len(LONG_PATH_PREFIX) * sizeof(wchar_t));
    memcpy(long_path.buffer + ws_len(LONG_PATH_PREFIX), t_info->path.buffer, len_to_size(t_info->path.len) * sizeof(wchar_t));
    *out = long_path;

    return SUCCESS;
}

error_code get_exe_version(const tool_info *t_info, version *out) {
    null_check(t_info, "get_exe_version");
    null_check(out, "get_exe_version");

    readonly_wstring exe_path;
    error_code ec;
    if ((ec = resolve_long_path(t_info, &exe_path)) != SUCCESS) {
        return ec;
    }

    version v;
    DWORD dummy;

    DWORD size = GetFileVersionInfoSizeW(exe_path.buffer, &dummy);
    if (size == 0) {
        println_error("get_exe_version: GetFileVersionInfoSizeW failed: %lu\n", GetLastError());
        return WINAPI_FUNCTION_ERROR;
    }

    void* buffer = malloc(size);
    if (!buffer) {
        println_error("get_exe_version: GetFileVersionInfoSizeW failed: %lu\n", GetLastError());
        return WINAPI_FUNCTION_ERROR;
    }

    if (GetFileVersionInfoW(exe_path.buffer, 0, size, buffer)) {
        VS_FIXEDFILEINFO* file_info = nullptr;
        UINT len = 0;

        if (VerQueryValueW(buffer, L"\\", (void**)&file_info, &len)) {
            v.major = HIWORD(file_info->dwFileVersionMS);
            v.minor = LOWORD(file_info->dwFileVersionMS);
            v.patch = HIWORD(file_info->dwFileVersionLS);
            v.build = LOWORD(file_info->dwFileVersionLS);
        }
    }

    free(exe_path.buffer);
    free(buffer);
    *out = (version){v.major, v.minor, v.patch, v.build, 4};
    return SUCCESS;
}

error_code path_combine(readonly_wstring path1, readonly_wstring path2, readonly_wstring *out) {
    null_check(path1.buffer, "path_combine");
    null_check(path2.buffer, "path_combine");
    null_check(out, "path_combine");

    size_t len1 = path1.len;
    size_t len2 = path2.len;
    size_t len = len1 + len2;
    bool add_backslash = false;

    if (path1.buffer[path1.len - 1] != L'\\' && path2.buffer[0] != L'\\') {
        ++len;
        add_backslash = true;
    }

    if (path1.buffer[path1.len - 1] == L'\\' && path2.buffer[0] == L'\\') {
        --len1;
        --len;
    }

    wchar_t *buffer = malloc(len_to_size(len) * sizeof(wchar_t));
    if (!buffer) {
        println_error("path_combine: malloc failure: %s", strerror(errno));
        return MALLOC_FAILURE;
    }

    size_t buffer_offset = 0;
    memcpy(buffer, path1.buffer, len1 * sizeof(wchar_t));
    buffer_offset += len1;

    if (add_backslash) {
        buffer[buffer_offset++] = L'\\';
    }

    memcpy(buffer + buffer_offset, path2.buffer, len_to_size(len2) * sizeof(wchar_t));

    *out = (readonly_wstring){buffer, len};

    return SUCCESS;
}

static error_code save_search_results(tool_info *t_info, wchar_t *path, size_t len, bool add_exe) {
    error_code ec;
    if (add_exe) {
        readonly_wstring exe;
        if ((ec = get_exe_string(t_info, &exe)) != SUCCESS) {
            free(path);
            return ec;
        }

        readonly_wstring old_exe = {nullptr, 0};
        if (t_info->folder.buffer) {
            old_exe = exe;
            if ((ec = path_combine(t_info->folder, exe, &exe)) != SUCCESS) {
                free(path);
                free(exe.buffer);
                return ec;
            }
        }

        if ((ec = path_combine((readonly_wstring){path, len}, exe, &t_info->path)) != SUCCESS) {
            free(path);
            free(exe.buffer);
            return ec;
        }
        free(exe.buffer);
        if (old_exe.buffer) {
            free(old_exe.buffer);
        }
        free(path);
    }
    else {
        t_info->path.buffer = path;
        t_info->path.len = len;
        t_info->has_long_path = false;
    }

    if (t_info->path.len >= MAX_PATH) {
        t_info->has_long_path = true;
    }

    readonly_wstring long_path;
    if ((ec = resolve_long_path(t_info, &long_path)) != SUCCESS) {
        return ec;
    }

    if (GetFileAttributesW(long_path.buffer) == INVALID_FILE_ATTRIBUTES) {
        if (!add_exe) {
            goto path_not_found;
        }
        if (t_info->folder.buffer) {
            goto path_not_found;
        }
        readonly_wstring bin = {L"bin", ws_len(L"bin")};

        readonly_wstring exe;
        if ((ec = get_exe_string(t_info, &exe)) != SUCCESS) {
            free(path);
            free(long_path.buffer);
            return ec;
        }

        if ((ec = path_combine((readonly_wstring){path, len}, bin, &t_info->path)) != SUCCESS) {
            free(path);
            free(exe.buffer);
            free(long_path.buffer);
            return ec;
        }
        if ((ec = path_combine(t_info->path, exe, &t_info->path)) != SUCCESS) {
            free(path);
            free(exe.buffer);
            free(long_path.buffer);
            return ec;
        }
        free(exe.buffer);
        free(long_path.buffer);

        if (t_info->path.len >= MAX_PATH) {
            t_info->has_long_path = true;
        }

        if ((ec = resolve_long_path(t_info, &long_path)) != SUCCESS) {
            goto rethrow;
        }

        if (GetFileAttributesW(long_path.buffer) == INVALID_FILE_ATTRIBUTES) {
            free(path);
            free(long_path.buffer);
            return PATH_NOT_FOUND;
        }
    }
    free(long_path.buffer);

    if (!t_info->use_version) {
        return SUCCESS;
    }

    version v;
    if ((ec = get_exe_version(t_info, &v)) != SUCCESS) {
        goto rethrow;
    }

    int cmp;
    if ((ec = cmp_version(t_info->version, v, &cmp)) != SUCCESS) {
        goto rethrow;
    }

    if (cmp == 0) {
        return SUCCESS;
    }

    free(path);
    t_info->path = (readonly_wstring){nullptr, 0};
    return VERSION_MISMATCH;

    rethrow:
    free(path);
    t_info->path = (readonly_wstring){nullptr, 0};
    return ec;

    path_not_found:
    free(path);
    free(long_path.buffer);
    return PATH_NOT_FOUND;
}

static error_code get_registry_entry(wchar_t *sub_key, wchar_t *value, wchar_t **out, size_t *out_len) {
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, sub_key, 0, KEY_READ, &key) != ERROR_SUCCESS) {
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, sub_key, 0, KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS) {
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, sub_key, 0, KEY_READ | KEY_WOW64_32KEY, &key) != ERROR_SUCCESS) {
                return PATH_NOT_FOUND;
            }
        }
    }

    DWORD byte_size = 0;
    if (RegGetValueW(key, nullptr, value, RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ, nullptr, nullptr, &byte_size) != ERROR_SUCCESS) {
        RegCloseKey(key);
        return PATH_NOT_FOUND;
    }

    LPBYTE path = malloc(byte_size);
    if (!path) {
        RegCloseKey(key);
        println_error("search_sub_key: malloc failure: %s", strerror(errno));
        return MALLOC_FAILURE;
    }

    if (RegGetValueW(key, nullptr, value, RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ, nullptr, path, &byte_size) != ERROR_SUCCESS) {
        RegCloseKey(key);
        free(path);
        return PATH_NOT_FOUND;
    }
    RegCloseKey(key);

    *out = (wchar_t*)path;
    *out_len = byte_size / sizeof(wchar_t) - 1;

    return SUCCESS;
}

error_code search_path(tool_info *t_info) {
    null_check(t_info, "search_path");

    DWORD required_len;
    readonly_wstring exe;
    error_code ec;
    if ((ec = get_exe_string(t_info, &exe)) != SUCCESS) {
        return ec;
    }
    if ((required_len = SearchPathW(nullptr, exe.buffer, nullptr, 0, nullptr, nullptr))) {
        wchar_t *path = malloc(len_to_size(required_len) * sizeof(wchar_t));
        if (!path) {
            free(exe.buffer);
            println_error("search_path: malloc failure: %s", strerror(errno));
            return MALLOC_FAILURE;
        }

        DWORD result = SearchPathW(nullptr, exe.buffer, nullptr, len_to_size(required_len), path, nullptr);
        free(exe.buffer);

        if (result == 0 || result  > required_len) {
            free(path);
            t_info->path = (readonly_wstring){nullptr, 0};
            return PATH_NOT_FOUND;
        }

        return save_search_results(t_info, path, result, false);
    }

    free(exe.buffer);
    return PATH_NOT_FOUND;
}

error_code search_app_paths(tool_info *t_info) {
    null_check(t_info, "search_app_paths");

    readonly_wstring app_paths = {L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\", ws_len(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\")};

    readonly_wstring exe;
    error_code ec;
    if ((ec = get_exe_string(t_info, &exe)) != SUCCESS) {
        return ec;
    }

    readonly_wstring search_path;
    if ((ec = path_combine(app_paths, exe, &search_path)) != SUCCESS) {
        free(exe.buffer);
        return ec;
    }
    free(exe.buffer);

    wchar_t *path = nullptr;
    size_t path_len = 0;
    if ((ec = get_registry_entry(search_path.buffer, nullptr, &path, &path_len)) != SUCCESS) {
        free(search_path.buffer);
        return ec;
    }
    free(search_path.buffer);
    return save_search_results(t_info, path, path_len, false);
}

error_code search_uninstall_paths(tool_info *t_info) {
    null_check(t_info, "search_uninstall_paths");

    typedef struct registry_location {
        HKEY h_root;
        LPCWSTR sub_key;
        REGSAM access;
    } registry_location;

    registry_location locations[] = {
        {HKEY_CURRENT_USER,  L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall", KEY_READ},
        {HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", KEY_READ | KEY_WOW64_64KEY},
        {HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", KEY_READ | KEY_WOW64_32KEY}
    };

    for (int i = 0; i < 3; ++i) {
        HKEY h_uninstall_key;
        if (RegOpenKeyExW(locations[i].h_root, locations[i].sub_key, 0, locations[i].access, &h_uninstall_key) != ERROR_SUCCESS) {
            continue;
        }

        DWORD index = 0;
        wchar_t sub_key_name[256];  // registry keys can have a maximum of 255 characters + null terminator, longer keys are impossible
        DWORD sub_key_name_len = 256;

        while (RegEnumKeyExW(h_uninstall_key, index++, sub_key_name, &sub_key_name_len, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
            sub_key_name_len = 256;

            HKEY key;
            if (RegOpenKeyExW(h_uninstall_key, sub_key_name, 0, locations[i].access, &key) != ERROR_SUCCESS) {
                RegCloseKey(key);
                continue;
            }

            wchar_t displayName[512];
            DWORD displayNameSize = sizeof(displayName);

            if (RegGetValueW(key, nullptr, L"DisplayName", RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ, nullptr, displayName, &displayNameSize) != ERROR_SUCCESS) {
                RegCloseKey(key);
                continue;
            }

            if (StrStrIW(displayName, t_info->tool_name.buffer) == nullptr) {
                RegCloseKey(key);
                continue;
            }

            DWORD byte_size = 0;
            if (RegGetValueW(key, nullptr, L"InstallLocation", RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ, nullptr, nullptr, &byte_size) != ERROR_SUCCESS) {
                RegCloseKey(key);
                continue;
            }

            LPBYTE install_path = malloc(byte_size);
            if (!install_path) {
                RegCloseKey(key);
                RegCloseKey(h_uninstall_key);
                return MALLOC_FAILURE;
            }

            if (RegGetValueW(key, nullptr, L"InstallLocation", RRF_RT_REG_SZ, nullptr, install_path, &byte_size) != ERROR_SUCCESS) {
                RegCloseKey(key);
                free(install_path);
                continue;
            }
            RegCloseKey(key);

            if (install_path[0] == L'\0') {
                free(install_path);
                continue;
            }
            RegCloseKey(h_uninstall_key);

            return save_search_results(t_info, (wchar_t*)install_path, byte_size / sizeof(wchar_t) - 1, true);
        }
        RegCloseKey(h_uninstall_key);
    }

    return PATH_NOT_FOUND;
}

error_code search_sub_key(tool_info *t_info) {
    null_check(t_info, "search_sub_key");

    if (!t_info->sub_key.buffer) {
        return PATH_NOT_FOUND;
    }

    readonly_wstring sub_key;
    error_code ec;
    if ((ec = sub_key_resolve_version(t_info, &sub_key)) != SUCCESS) {
        return ec;
    }

    wchar_t *path = nullptr;
    size_t path_len = 0;
    if ((ec = get_registry_entry(sub_key.buffer, t_info->value.buffer, &path, &path_len)) != SUCCESS) {
        free(sub_key.buffer);
        return ec;
    }
    free(sub_key.buffer);

    return save_search_results(t_info, path, path_len, true);
}

error_code search_tool(tool_info *t_info) {
    null_check(t_info, "search_tool");

    if (search_sub_key(t_info) == SUCCESS) {
        return SUCCESS;
    }
    if (search_app_paths(t_info) == SUCCESS) {
        return SUCCESS;
    }
    if (search_uninstall_paths(t_info) == SUCCESS) {
        return SUCCESS;
    }
    return search_path(t_info);
}