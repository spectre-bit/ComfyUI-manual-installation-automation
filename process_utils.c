/*
some utility to create processes
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
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <wchar.h>

#include "path_utils.h"
#include "process_utils.h"

error_code init_process(process *p) {
    *p = (process){};

    p->si = (STARTUPINFOW){};
    p->pi = (PROCESS_INFORMATION){};

    p->si.cb = sizeof(STARTUPINFOW);

    return SUCCESS;
}

error_code discard_io(process *p) {
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), nullptr, true};
    HANDLE h_nul = CreateFileW(L"NUL", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h_nul == INVALID_HANDLE_VALUE) {
        println_error("CreateFileW failure: %lu", GetLastError());
        return WINAPI_FUNCTION_ERROR;
    }

    p->si.hStdInput  = h_nul;
    p->si.hStdOutput = h_nul;
    p->si.hStdError  = h_nul;
    p->si.dwFlags |= STARTF_USESTDHANDLES;

    return SUCCESS;
}

error_code pipe(process *p) {
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), nullptr, true};

    if (!CreatePipe(&p->child_stdout_rd, &p->child_stdout_wr, &sa, 0)) {
        println_error("CreatePipe failure: %lu", GetLastError());
        return WINAPI_FUNCTION_ERROR;
    }

    if (!SetHandleInformation(p->child_stdout_rd, HANDLE_FLAG_INHERIT, 0)) {
        println_error("SetHandleInformation failure: %lu", GetLastError());
        return WINAPI_FUNCTION_ERROR;
    }

    HANDLE h_nul = CreateFileW(L"NUL", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h_nul == INVALID_HANDLE_VALUE) {
        println_error("CreateFileW failure: %lu", GetLastError());
        return WINAPI_FUNCTION_ERROR;
    }
    p->child_stdin = h_nul;

    p->si.hStdOutput = p->child_stdout_wr;
    p->si.hStdError = p->child_stdout_wr;
    p->si.hStdInput = p->child_stdin;
    p->si.dwFlags |= STARTF_USESTDHANDLES;

    return SUCCESS;
}

error_code execute_s(process *p, tool_info *t_info, readonly_wstring args) {
    readonly_wstring long_path;
    error_code ec;
    if ((ec = resolve_long_path(t_info, &long_path)) != SUCCESS) {
        return ec;
    }

    size_t cli_args_len = long_path.len + 3 + args.len;
    wchar_t *cli_args = malloc(len_to_size(cli_args_len) * sizeof(wchar_t));
    if (!cli_args) {
        free(long_path.buffer);
        println_error("execute: malloc failure: %s", strerror(errno));
        return MALLOC_FAILURE;
    }

    size_t offset = 0;
    cli_args[offset++] = L'"';
    memcpy(cli_args + offset, long_path.buffer, long_path.len * sizeof(wchar_t));
    offset += long_path.len;
    cli_args[offset++] = L'"';
    cli_args[offset++] = L' ';
    memcpy(cli_args + offset, args.buffer, len_to_size(args.len) * sizeof(wchar_t));

    HANDLE h_job = CreateJobObjectW(nullptr, nullptr);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = {0};
    jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    SetInformationJobObject(h_job, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));
    p->h_job = h_job;

    if (CreateProcessW(long_path.buffer, cli_args, nullptr, nullptr, TRUE, CREATE_SUSPENDED, nullptr, p->working_directory, &p->si, &p->pi)) {
        AssignProcessToJobObject(h_job, p->pi.hProcess);
        ResumeThread(p->pi.hThread);

        CloseHandle(p->child_stdout_wr);
        CloseHandle(p->child_stdin);
        free(cli_args);
        free(long_path.buffer);
        return SUCCESS;
    }

    free(cli_args);
    free(long_path.buffer);
    println_error("CreateProcessW failure: %lu", GetLastError());
    return WINAPI_FUNCTION_ERROR;
}

error_code loading_animation_s(wchar_t *text, size_t len, HANDLE h_console) {
    static size_t offset = 0;

    wchar_t block = L'█';
    size_t length = 20;
    size_t gap = 5;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(h_console, &csbi)) {
        return WINAPI_FUNCTION_ERROR;
    }

    size_t width = (size_t)csbi.dwSize.X;

    if (width <= len + gap) {
        return INVALID_ARGUMENT;
    }

    size_t available_width = width - len - gap;
    offset %= available_width;

    wchar_t *buffer = malloc((width + 1) * sizeof(wchar_t));
    if (!buffer) {
        return MALLOC_FAILURE;
    }

    wmemset(buffer, L' ', width);

    size_t o = 0;
    memcpy(buffer, text, len * sizeof(wchar_t));
    o += len + gap;

    if (offset + length > available_width) {
        size_t wrap_amount = (offset + length) - available_width;

        for (size_t i = 0; i < wrap_amount; ++i) {
            buffer[o++] = block;
        }

        length -= wrap_amount;
    }

    o = len + gap + offset;

    for (size_t i = 0; i < length; ++i) {
        buffer[o++] = block;
    }

    buffer[width] = L'\0';

    DWORD written = 0;
    WriteConsoleW(h_console, L"\r", 1, &written, NULL);
    WriteConsoleW(h_console, buffer, width, &written, NULL);

    offset += 5;
    offset %= available_width;

    free(buffer);

    return SUCCESS;
}