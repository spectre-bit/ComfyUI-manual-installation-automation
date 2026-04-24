/*
starts ComfyUI and opens the website when the server is running
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

#pragma comment(linker, "/STACK:16384")
#define _CRT_SECURE_NO_WARNINGS

#ifdef _DEBUG
    #define _CRTDBG_MAP_ALLOC
    #include <crtdbg.h>
#endif

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <limits.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <shellapi.h>

#include "path_utils.h"
#include "process_utils.h"
#include "utils.h"

int wmain() {
#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
#endif

    tool_info t_info;
    error_code ec;
    init_tool_info(&t_info);
    readonly_wstring python_path;
    if ((ec = to_absolute_path(L"comfy-env\\Scripts\\python.exe", &python_path)) != SUCCESS) {
        println_error("to_absolute_path error");
        return ec;
    }
    t_info.path = python_path;

    wchar_t args_file[] = L"args.ini";

    readonly_wstring path;
    if ((ec = get_own_path(&path)) != SUCCESS) {
        println_error("failed to get path of own file");
        return ec;
    }

    wchar_t *file = wcsrchr(path.buffer, L'\\');
    path.len = file - path.buffer;

    readonly_wstring args_ini;
    if ((ec = path_combine(path, (readonly_wstring){args_file, ws_len(args_file)}, &args_ini)) != SUCCESS) {
        println_error("path combine error");
        return ec;
    }
    free(path.buffer);

    readonly_wstring args;
    if ((ec = get_ini_string_arg(args_ini.buffer, L"cli_args", &args)) != SUCCESS) {
        println_error("error getting cli-args from args.ini");
        return ec;
    }
    free(args_ini.buffer);

    int port = 8188;

    wchar_t *port_arg = wcsstr(args.buffer, L"--port");
    if (port_arg) {
        port_arg += ws_len(L"--port");
        while (*port_arg == L' ') {
            ++port_arg;
        }

        errno = 0;
        wchar_t *end;
        port = wcstol(port_arg, &end, 10);
        if (errno == ERANGE) {
            if (port == LONG_MAX) {
                println_error("error: invalid port argument: long overflow for port argument");
                return INVALID_ARGUMENT;
            }
            println_error("error: invalid port argument: long underflow for port argument");
            return INVALID_ARGUMENT;
        }

        if (end == port_arg) {
            println_error("error: invalid port argument: argument must be a numeric value");
            return INVALID_ARGUMENT;
        }

        if (port < 0 || port > USHRT_MAX) {
            println_error("error: invalid port argument: invalid port number\nonly number from 0 to 65535 are valid");
            return INVALID_ARGUMENT;
        }
    }

    readonly_wstring full_args;
    if ((ec = wstr_cat(ws_len(L" main.py ") + args.len + t_info.path.len, &full_args, 3, t_info.path, (readonly_wstring){L" main.py ", ws_len(L" main.py ")}, args)) != SUCCESS) {
        println_error("wstr_cat error");
        return ec;
    }
    free(args.buffer);

    PROCESS_INFORMATION pi = {};
    STARTUPINFOW si = {sizeof(STARTUPINFOW)};

    if (!CreateProcessW(t_info.path.buffer, full_args.buffer, nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi)) {
        println_error("CreateProcess error: %lu", GetLastError());
        return WINAPI_FUNCTION_ERROR;
    }
    free(full_args.buffer);

    int len = swprintf(nullptr, 0, L"http://127.0.0.1:%d", port);
    wchar_t *url = malloc(len_to_size(len) * sizeof(wchar_t));
    if (!url) {
        println_error("malloc error: %s", strerror(errno));
        return MALLOC_FAILURE;
    }
    swprintf(url, len_to_size(len), L"http://127.0.0.1:%d", port);

    SOCKET s;
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        return WINAPI_FUNCTION_ERROR;
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);

    if (InetPtonW(AF_INET, L"127.0.0.1", &addr.sin_addr) != 1) {
        println_error("InetPton error: %lu", GetLastError());
        return WINAPI_FUNCTION_ERROR;
    }

    bool connected = false;

    // wait till the ComfyUI server is running
    while (!connected) {
        s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s == INVALID_SOCKET) {
            break;
        }

        if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            connected = true;
        }
        closesocket(s);

        if (connected) {
            Sleep(100);
        }
        else {
            Sleep(200);
        }
    }

    WSACleanup();
    ShellExecuteW(nullptr, L"open", url, nullptr, nullptr, SW_SHOWNORMAL);
    free(url);

    SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return SUCCESS;
}