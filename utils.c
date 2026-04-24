/*
some utility functions
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

#include "utils.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <mmsystem.h>
#include <pathcch.h>
#include <wchar.h>
#include <shlobj.h>

#include "path_utils.h"
#include "process_utils.h"

error_code println_fill_width(readonly_wstring text, HANDLE h_console) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(h_console, &csbi)) {
        return WINAPI_FUNCTION_ERROR;
    }

    int console_width = csbi.dwSize.X;
    DWORD written;
    WriteConsoleW(h_console, L"\r", 1, &written, nullptr);
    COORD coord = {0, csbi.dwCursorPosition.Y};
    FillConsoleOutputCharacterW(h_console, L' ', console_width, coord, &written);
    WriteConsoleW(h_console, text.buffer, text.len, &written, nullptr);
    WriteConsoleW(h_console, L"\n", 1, &written, nullptr);

    return SUCCESS;
}

error_code get_latest_tag(tool_info *t_info, process *p, readonly_wstring *tag_out, HANDLE h_out) {
    init_process(p);
    pipe(p);
    error_code ec;

    if ((ec = execute(p, t_info, L"ls-remote --tags --sort=\"-version:refname\" https://github.com/Comfy-Org/ComfyUI.git")) != SUCCESS) {
        println_error("execute failed");
        free(t_info->path.buffer);
        return ec;
    }

    size_t size = 1024;
    char *buffer = malloc(size);
    size_t offset = 0;
    char *first_line = nullptr;

    printf("Getting latest release tag");
    while (true) {
        DWORD bytes_read;
        bool success = ReadFile(p->child_stdout_rd, buffer + offset, size - offset, &bytes_read, nullptr);
        offset += bytes_read;

        if ((success && bytes_read == 0) || (!success && GetLastError() == ERROR_BROKEN_PIPE)) {
            free(buffer);
            CloseHandle(p->child_stdout_rd);
            break;
        }
        if (!success) {
            println_error("ReadFile failed: %lu", GetLastError());
            free(buffer);
            free(t_info->path.buffer);
            return WINAPI_FUNCTION_ERROR;
        }

        char *newline;
        if ((newline = strchr(buffer, '\n'))) {
            first_line = malloc(newline - buffer + 1);
            if (!first_line) {
                free(buffer);
                free(t_info->path.buffer);
                println_error("malloc failure: %s", strerror(errno));
                return MALLOC_FAILURE;
            }
            memcpy(first_line, buffer, newline - buffer);
            first_line[newline - buffer] = '\0';
            free(buffer);
            CloseHandle(p->child_stdout_rd);
            break;
        }

        if (offset == size) {
            char *temp = realloc(buffer, size *= 2);
            if (!temp) {
                free(buffer);
                free(t_info->path.buffer);
                println_error("realloc failure: %s", strerror(errno));
                return REALLOC_FAILURE;
            }
            buffer = temp;
        }
    }
    CloseHandle(p->pi.hProcess);
    CloseHandle(p->pi.hThread);
    CloseHandle(p->h_job);

    if (!first_line) {
        goto error_fetching_tag;
    }

    char *tag = strstr(first_line, "tags/");
    tag += 5;
    size_t tag_len = strlen(tag);

    if (!tag) {
        goto error_fetching_tag;
    }

    println_success_fill("Got latest ComfyUI release tag", h_out);

    int wsize = MultiByteToWideChar(CP_UTF8, 0, tag, len_to_size(tag_len), nullptr, 0);
    wchar_t *wtag = malloc(wsize * sizeof(wchar_t));
    if (!wtag) {
        free(first_line);
        println_error("malloc failure: %s", strerror(errno));
        return MALLOC_FAILURE;
    }
    if (MultiByteToWideChar(CP_UTF8, 0, tag, len_to_size(tag_len), wtag, wsize) == 0) {
        println_error("MultiByteToWideChar failed: %lu", GetLastError());
        free(wtag);
        free(first_line);
        return WINAPI_FUNCTION_ERROR;
    }

    *tag_out = (readonly_wstring){wtag, size_to_len(wsize)};
    free(first_line);
    return SUCCESS;

    error_fetching_tag:
    println_error("error fetching the latest release tag from ComfyUI, aborting program");
    return ERROR_FETCHING_TAG;
}

error_code wait_with_animation(HANDLE h_job, HANDLE h_process, bool is_console, HANDLE h_out, readonly_wstring text, readonly_wstring success_text, readonly_wstring failure_text) {
    if (!is_console) {
        JOBOBJECT_BASIC_ACCOUNTING_INFORMATION hai;
        do {
            QueryInformationJobObject(h_job, JobObjectBasicAccountingInformation, &hai, sizeof(hai), nullptr);
            Sleep(100);
        } while (hai.ActiveProcesses > 0);
    }
    else {
        CONSOLE_CURSOR_INFO cursorInfo;
        if (!GetConsoleCursorInfo(h_out, &cursorInfo)) {
            println_error("GetConsoleCursorInfo failed: %lu", GetLastError());
            CloseHandle(h_job);
            return WINAPI_FUNCTION_ERROR;
        }
        cursorInfo.bVisible = false;
        if (!SetConsoleCursorInfo(h_out, &cursorInfo)) {
            println_error("SetConsoleCursorInfo failed: %lu", GetLastError());
            CloseHandle(h_job);
            return WINAPI_FUNCTION_ERROR;
        }

        long long duration = 0;

        JOBOBJECT_BASIC_ACCOUNTING_INFORMATION hai;
        do {
            timeBeginPeriod(1);

            LARGE_INTEGER frequency, start, end;
            QueryPerformanceFrequency(&frequency);

            QueryInformationJobObject(h_job, JobObjectBasicAccountingInformation, &hai, sizeof(hai), nullptr);

            QueryPerformanceCounter(&start);
            loading_animation_s(text.buffer, text.len, h_out);
            QueryPerformanceCounter(&end);
            duration = (end.QuadPart - start.QuadPart) * 1000 / frequency.QuadPart;

            Sleep(33 - duration);
        } while (hai.ActiveProcesses > 0);

        CloseHandle(h_job);

        cursorInfo.bVisible = true;
        SetConsoleCursorInfo(h_out, &cursorInfo);

        DWORD exit_code;
        if (!GetExitCodeProcess(h_process, &exit_code)) {
            println_error("GetExitCodeProcess failed: %lu", GetLastError());
            return WINAPI_FUNCTION_ERROR;
        }

        if (exit_code == 0) {
            DWORD written;
            if (!WriteConsoleW(h_out, L_GREEN, ws_len(L_GREEN), &written, nullptr)) {
                println_error("WriteConsoleW failed: %lu", GetLastError());
                return WINAPI_FUNCTION_ERROR;
            }
            println_fill_width(success_text, h_out);
            if (!WriteConsoleW(h_out, L_RESET, ws_len(L_RESET), &written, nullptr)) {
                println_error("WriteConsoleW failed: %lu", GetLastError());
                return WINAPI_FUNCTION_ERROR;
            }
        }
        else {
            DWORD written;
            if (!WriteConsoleW(h_out, L_RED, ws_len(L_RED), &written, nullptr)) {
                println_error("WriteConsoleW failed: %lu", GetLastError());
                return WINAPI_FUNCTION_ERROR;
            }
            println_fill_width(failure_text, h_out);
            if (!WriteConsoleW(h_out, L_RESET, ws_len(L_RESET), &written, nullptr)) {
                println_error("WriteConsoleW failed: %lu", GetLastError());
                return WINAPI_FUNCTION_ERROR;
            }
            return FAILURE;
        }
    }

    return SUCCESS;
}


error_code install_git(bool is_console, HANDLE h_out, readonly_wstring text) {
    tool_info t_info;
    init_tool_info(&t_info);
    t_set_name(&t_info, L"winget");

    error_code ec;
    if ((ec = search_tool(&t_info)) != SUCCESS) {
        println_error("unable to find winget");
        return ec;
    }

    process p;
    init_process(&p);
    discard_io(&p);

    if ((ec = execute(&p, &t_info, L"install --exact --id Git.Git --silent --accept-package-agreements --accept-source-agreements --scope user")) != SUCCESS) {
        return ec;
    }
    if ((ec = wait_with_animation(p.h_job, p.pi.hProcess, is_console, h_out, text, (readonly_wstring){L"Successfully installed git", ws_len(L"Successfully installed git")}, (readonly_wstring){L"Failed to install git", ws_len(L"Failed to install git")})) != SUCCESS) {
        if (ec == FAILURE) {
            DWORD exit_code = 0;
            if (!GetExitCodeProcess(p.pi.hProcess, &exit_code)) {
                println_error("GetExitCodeProcess failed: %lu", GetLastError());
                return WINAPI_FUNCTION_ERROR;
            }

            CloseHandle(p.pi.hProcess);
            CloseHandle(p.pi.hThread);

            if (exit_code == 2316632332) {
                println_error("User denied elevation of git installer\nUnable to install git, unable to clone comfyUI without git, aborting program, comfyUI was not installed");
                return ELEVATION_DENIED;
            }
            if (exit_code != 0) {
                println_error("The winget process got an error and ended with exit code: %lu", exit_code);
                return FAILURE;
            }
        }
    }

    return SUCCESS;
}

error_code install_python(bool is_console, HANDLE h_out, readonly_wstring text, version v) {
    tool_info t_info;
    init_tool_info(&t_info);
    t_set_name(&t_info, L"winget");

    error_code ec;
    if ((ec = search_tool(&t_info)) != SUCCESS) {
        println_error("unable to find winget");
        return ec;
    }

    process p;
    init_process(&p);
    discard_io(&p);

    size_t len = ws_len(L"install --scope user --silent --accept-package-agreements --accept-source-agreements -e --id Python.Python.");
    readonly_wstring version;
    v.how_many = 2;
    if ((ec = get_version_string(v, &version)) != SUCCESS) {
        free(t_info.path.buffer);
        println_error("install_python: get_version_string error");
        return ec;
    }
    wchar_t *cmd = malloc((len + len_to_size(version.len)) * sizeof(wchar_t));
    if (!cmd) {
        free(t_info.path.buffer);
        println_error("malloc failure: %s", strerror(errno));
        return MALLOC_FAILURE;
    }
    memcpy(cmd, L"install --scope user --silent --accept-package-agreements --accept-source-agreements -e --id Python.Python.", len * sizeof(wchar_t));
    memcpy(cmd + len, version.buffer, len_to_size(version.len) * sizeof(wchar_t));
    readonly_wstring cmd_text = {cmd, len + version.len};

    size_t success_len = ws_len(L"Successfully installed python ");
    wchar_t *success = malloc((success_len + len_to_size(version.len)) * sizeof(wchar_t));
    if (!success) {
        free(cmd);
        free(t_info.path.buffer);
        println_error("malloc failure: %s", strerror(errno));
        return MALLOC_FAILURE;
    }
    memcpy(success, L"Successfully installed python ", success_len * sizeof(wchar_t));
    memcpy(success + success_len, version.buffer, len_to_size(version.len) * sizeof(wchar_t));
    readonly_wstring success_text = {success, success_len + version.len};

    size_t failure_len = ws_len(L"Failed to install python ");
    wchar_t *failure = malloc((failure_len + len_to_size(version.len)) * sizeof(wchar_t));
    if (!failure) {
        free(success);
        free(cmd);
        free(t_info.path.buffer);
        println_error("malloc failure: %s", strerror(errno));
        return MALLOC_FAILURE;
    }
    memcpy(failure, L"Failed to install python " , failure_len * sizeof(wchar_t));
    memcpy(failure + failure_len, version.buffer, len_to_size(version.len) * sizeof(wchar_t));
    readonly_wstring failure_text = {failure, failure_len + version.len};


    if ((ec = execute_s(&p, &t_info, cmd_text)) != SUCCESS) {
        free(t_info.path.buffer);
        free(cmd);
        free(success);
        free(failure);
        println_error("execute_s error");
        return ec;
    }
    free(t_info.path.buffer);
    if ((ec = wait_with_animation(p.h_job, p.pi.hProcess, is_console, h_out, text, success_text, failure_text)) != SUCCESS && ec != FAILURE) {
        println_error("error during animation");
        free(cmd);
        free(success);
        free(failure);
        CloseHandle(p.pi.hProcess);
        CloseHandle(p.pi.hThread);
        return ec;
    }

    CloseHandle(p.pi.hProcess);
    CloseHandle(p.pi.hThread);

    free(cmd);
    free(success);
    free(failure);
    return SUCCESS;
}

error_code init_console(HANDLE *h_out, bool *is_console) {
    *h_out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dw_mode = 0;
    *is_console = true;

    if (*h_out == INVALID_HANDLE_VALUE) {
        println_error("stdout is an invalid handle");
        return INVALID_HANDLE;
    }

    if (GetFileType(*h_out) != FILE_TYPE_CHAR) {
        *is_console = false;
    }

    if (*is_console) {
        if (!GetConsoleMode(*h_out, &dw_mode)) {
            println_error("GetConsoleMode failed: %lu", GetLastError());
            return WINAPI_FUNCTION_ERROR;
        }
        SetConsoleMode(*h_out, dw_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }

    return SUCCESS;
}


error_code get_own_path(readonly_wstring *out) {
    constexpr size_t MAX_LONG_PATH = 32767;
    out->buffer = malloc(MAX_LONG_PATH * sizeof(wchar_t));
    if (!out->buffer) {
        println_error("malloc failure: %s", strerror(errno));
        return MALLOC_FAILURE;
    }

    DWORD written = GetModuleFileNameW(nullptr, out->buffer, MAX_LONG_PATH);
    if (written == 0) {
        println_error("GetModuleFileNameW failed: %lu", GetLastError());
        free(out->buffer);
        return WINAPI_FUNCTION_ERROR;
    }

    constexpr size_t PREFIX_LEN = ws_len(L"\\\\?\\");
    if (written >= MAX_PATH && wcsncmp(out->buffer, L"\\\\?\\", PREFIX_LEN) != 0) {
        memmove(out->buffer + PREFIX_LEN, out->buffer, len_to_size(written) * sizeof(wchar_t));
        out->buffer[0] = L'\\';
        out->buffer[1] = L'\\';
        out->buffer[2] = L'?';
        out->buffer[3] = L'\\';

        wchar_t *temp = realloc(out->buffer, (len_to_size(written) + PREFIX_LEN) * sizeof(wchar_t));
        if (!temp) {
            println_error("realloc failure: %s", strerror(errno));
            free(out->buffer);
            return REALLOC_FAILURE;
        }
        out->buffer = temp;
        out->len = written + PREFIX_LEN;
    }
    else {
        wchar_t *temp = realloc(out->buffer, len_to_size(written) * sizeof(wchar_t));
        if (!temp) {
            println_error("realloc failure: %s", strerror(errno));
            free(out->buffer);
            return REALLOC_FAILURE;
        }
        out->buffer = temp;
        out->len = written;
    }

    return SUCCESS;
}

error_code get_ini_string_arg(wchar_t *path, wchar_t *key, readonly_wstring *out) {
    out->buffer = malloc(1024 * sizeof(wchar_t));
    if (!out->buffer) {
        println_error("malloc failure: %s", strerror(errno));
        return MALLOC_FAILURE;
    }
    out->len = 1024;

    get_value:
    DWORD written = GetPrivateProfileStringW(L"Arguments", key, nullptr, out->buffer, out->len, path);
    if (written == 0) {
        println_error("GetPrivateProfileStringW failed: %lu", GetLastError());
        free(out->buffer);
        return WINAPI_FUNCTION_ERROR;
    }

    if (written == out->len - 1) {
        out->len *= 2;
        wchar_t *temp = realloc(out->buffer, out->len * sizeof(wchar_t));
        if (!temp) {
            println_error("realloc failure: %s", strerror(errno));
            free(out->buffer);
            return REALLOC_FAILURE;
        }
        out->buffer = temp;
        goto get_value;
    }

    out->len = written;
    wchar_t *temp = realloc(out->buffer, len_to_size(out->len) * sizeof(wchar_t));
    if (!temp) {
        println_error("realloc failure: %s", strerror(errno));
        free(out->buffer);
        return REALLOC_FAILURE;
    }
    out->buffer = temp;

    return SUCCESS;
}

error_code to_absolute_path(wchar_t *path, readonly_wstring *out) {
    constexpr wchar_t long_path_prefix[] = L"\\\\?\\";

    DWORD size = GetFullPathNameW(path, 0, nullptr, nullptr);
    if (size == 0) {
        println_error("GetFullPathNameW failed: %lu", GetLastError());
        return WINAPI_FUNCTION_ERROR;
    }
    out->len = size - 1;
    size_t offset = 0;
    if (size >= MAX_PATH) {
        out->len += ws_len(long_path_prefix);
        offset += ws_len(long_path_prefix);
    }
    out->buffer = malloc(len_to_size(out->len) * sizeof(wchar_t));
    if (!out->buffer) {
        println_error("malloc failure: %s", strerror(errno));
        return MALLOC_FAILURE;
    }

    size = GetFullPathNameW(path, len_to_size(out->len) - offset, out->buffer + offset, nullptr);
    if (size == 0) {
        println_error("GetFullPathNameW failed: %lu", GetLastError());
        free(out->buffer);
        return WINAPI_FUNCTION_ERROR;
    }
    if (size >= MAX_PATH) {
        memcpy(out->buffer, long_path_prefix, ws_len(long_path_prefix) * sizeof(wchar_t));
    }

    return SUCCESS;
}