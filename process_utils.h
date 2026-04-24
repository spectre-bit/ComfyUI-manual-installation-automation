/*
the header file for process_utils.c
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

#ifndef CPPTEST_PROCESS_UTILS_H
#define CPPTEST_PROCESS_UTILS_H

#include <windows.h>

#include "path_utils.h"

typedef struct process {
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sec_attrs;
    HANDLE child_stdout_rd;
    HANDLE child_stdout_wr;
    HANDLE child_stdin;
    HANDLE h_job;
    wchar_t *working_directory;
} process;

error_code init_process(process *p);

error_code discard_io(process *p);

error_code pipe(process *p);

error_code execute_s(process *p, tool_info *t_info, readonly_wstring args);

#define execute(process, t_info, args) execute_s(process, t_info, (readonly_wstring){args, ws_len(args)})

error_code loading_animation_s(wchar_t *text, size_t len, HANDLE h_console);

#define loading_animation(text, h_console) loading_animation_s(text, ws_len(text), h_console)

#endif //CPPTEST_PROCESS_UTILS_H