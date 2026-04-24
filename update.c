/*
update uptades a manual comfyUI install to the newest or the specified version
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
#include <stdio.h>
#include <wchar.h>

#include "path_utils.h"
#include "process_utils.h"
#include "utils.h"

// update [--tag <tag>] [--new-manager]
int wmain(int argc, wchar_t* argv[]) {
#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
#endif

    HANDLE h_out;
    bool is_console;
    error_code ec;

    if ((ec = init_console(&h_out, &is_console)) != SUCCESS) {
        return ec;
    }

    if (argc > 4) {
        println_error("too many arguments.");
        return TOO_MANY_ARGUMENTS;
    }

    bool arg_tag = false;
    bool new_manager = false;
    readonly_wstring tag;

    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            if (wcscmp(argv[i], L"--tag") == 0) {
                if (arg_tag) {
                    println_error("--tag already specified");
                    return INVALID_ARGUMENT;
                }
                if (i == argc - 1) {
                    println_error("missing tag\nused --tag but got no ComfyUI release tag\nUsage: --tag v<major>.<minor>.<patch>\ne.g. --tag v0.12.3");
                    return INVALID_ARGUMENT;
                }
                int dummy;
                if (swscanf(argv[++i], L"v%*d.%*d.%d", &dummy) != 1) {
                    println_error("after --tag has to be a valid ComfyUI release tag, which have the following format: v<major>.<minor>.<patch>");
                    return INVALID_ARGUMENT;
                }
                tag.buffer = argv[i];
                tag.len = wcslen(argv[i]);
                arg_tag = true;
            }
            else if (wcscmp(argv[i], L"--new-manager") == 0) {
                if (new_manager) {
                    println_error("--new-manager already set");
                    return INVALID_ARGUMENT;
                }
                new_manager = true;
            }
            else {
                println_error("unknown argument: %ls", argv[i]);
                return INVALID_ARGUMENT;
            }
        }
    }

    tool_info t_info;
    process p;

    init_tool_info(&t_info);
    t_set_name(&t_info, L"git");
    t_set_sub_key(&t_info, L"SOFTWARE\\GitForWindows");
    t_set_folder(&t_info, L"cmd");

    if ((ec = search_tool(&t_info)) != SUCCESS) {
        puts(RED "Git not found" RESET);
        if ((ec = install_git(is_console, h_out, (readonly_wstring){L"Installing git", ws_len(L"Installing git")})) != SUCCESS) {
            return ec;
        }
        if ((ec = search_tool(&t_info)) != SUCCESS) {
            println_error("Git not found");
            return ec;
        }
    }
    else {
        puts(GREEN "Found git" RESET);
    }

    init_process(&p);
    pipe(&p);

    if (!arg_tag) {
        if ((ec = get_latest_tag(&t_info, &p, &tag, h_out)) != SUCCESS) {
            println_error("get_latest_tag failed");
            free(t_info.path.buffer);
            return ec;
        }
    }

    init_process(&p);
    discard_io(&p);

    wchar_t part1[] = L"fetch --depth 1 origin refs/tags/";
    size_t len_part1 = ws_len(part1);

    wchar_t part2[] = L":refs/tags/";
    size_t len_part2 = ws_len(part2);

    readonly_wstring args;
    if ((ec = wstr_cat(len_part1 + len_part2 + 2 * tag.len, &args, 4, (readonly_wstring){part1, len_part1}, tag, (readonly_wstring){part2, len_part2}, tag)) != SUCCESS) {
        println_error("wstr_cat failed");
        free(t_info.path.buffer);
        if (!arg_tag) {
            free(tag.buffer);
        }
        return ec;
    }

    if ((ec = execute_s(&p, &t_info, args)) != SUCCESS) {
        println_error("execute_s failed");
        free(t_info.path.buffer);
        free(args.buffer);
        if (!arg_tag) {
            free(tag.buffer);
        }
        return ec;
    }
    free(args.buffer);

    wchar_t text_part[] = L"Fetching ComfyUI tag ";
    size_t len_text_part = ws_len(text_part);

    wchar_t success_part[] = L"Successfully fetched ComfyUI tag ";
    size_t len_success_part = ws_len(success_part);

    wchar_t failure_part[] = L"Failed to fetch ComfyUI tag ";
    size_t len_failure_part = ws_len(failure_part);

    readonly_wstring text;
    if ((ec = wstr_cat(len_text_part + tag.len, &text, 2, (readonly_wstring){text_part, len_text_part}, tag)) != SUCCESS) {
        println_error("wstr_cat failed");
        free(t_info.path.buffer);
        if (!arg_tag) {
            free(tag.buffer);
        }
        return ec;
    }

    readonly_wstring success;
    if ((ec = wstr_cat(len_success_part + tag.len, &success, 2, (readonly_wstring){success_part, len_success_part}, tag)) != SUCCESS) {
        println_error("wstr_cat failed");
        free(t_info.path.buffer);
        free(text.buffer);
        if (!arg_tag) {
            free(tag.buffer);
        }
        return ec;
    }

    readonly_wstring failure;
    if ((ec = wstr_cat(len_failure_part + tag.len, &failure, 2, (readonly_wstring){failure_part, len_failure_part}, tag)) != SUCCESS) {
        println_error("wstr_cat failed");
        free(t_info.path.buffer);
        free(text.buffer);
        free(success.buffer);
        if (!arg_tag) {
            free(tag.buffer);
        }
        return ec;
    }

    if ((ec = wait_with_animation(p.h_job, p.pi.hProcess, is_console, h_out, text, success, failure)) != SUCCESS) {
        free(t_info.path.buffer);
        free(text.buffer);
        free(success.buffer);
        free(failure.buffer);
        if (!arg_tag) {
            free(tag.buffer);
        }
        return ec;
    }
    CloseHandle(p.pi.hProcess);
    CloseHandle(p.pi.hThread);
    free(text.buffer);
    free(success.buffer);
    free(failure.buffer);

    wchar_t checkout[] = L"checkout ";
    size_t len_checkout = ws_len(checkout);

    if ((ec = wstr_cat(len_checkout + tag.len, &args, 2, (readonly_wstring){checkout, len_checkout}, tag)) != SUCCESS) {
        println_error("wstr_cat failed");
        free(t_info.path.buffer);
        if (!arg_tag) {
            free(tag.buffer);
        }
        return ec;
    }

    if ((ec = execute_s(&p, &t_info, args)) != SUCCESS) {
        println_error("execute_s failed");
        free(t_info.path.buffer);
        free(args.buffer);
        if (!arg_tag) {
            free(tag.buffer);
        }
        return ec;
    }
    free(args.buffer);

    wchar_t checkout_text_part[] = L"Checking out to tag ";
    size_t len_checkout_tex_part = ws_len(checkout_text_part);

    wchar_t checkout_success_part[] = L"Successfully checked out to tag ";
    size_t len_checkout_success_part = ws_len(checkout_success_part);

    wchar_t checkout_failure_part[] = L"Failed to checked out to tag ";
    size_t len_checkout_failure_part = ws_len(checkout_failure_part);

    readonly_wstring checkout_text;
    if ((ec = wstr_cat(len_checkout_tex_part + tag.len, &checkout_text, 2, (readonly_wstring){checkout_text_part, len_checkout_tex_part}, tag)) != SUCCESS) {
        println_error("wstr_cat failed");
        free(t_info.path.buffer);
        if (!arg_tag) {
            free(tag.buffer);
        }
        return ec;
    }

    readonly_wstring checkout_success;
    if ((ec = wstr_cat(len_checkout_success_part + tag.len, &checkout_success, 2, (readonly_wstring){checkout_success_part, len_checkout_success_part}, tag)) != SUCCESS) {
        println_error("wstr_cat failed");
        free(t_info.path.buffer);
        free(checkout_text.buffer);
        if (!arg_tag) {
            free(tag.buffer);
        }
        return ec;
    }

    readonly_wstring checkout_failure;
    if ((ec = wstr_cat(len_checkout_failure_part + tag.len, &checkout_failure, 2, (readonly_wstring){checkout_failure_part, len_checkout_failure_part}, tag)) != SUCCESS) {
        println_error("wstr_cat failed");
        free(t_info.path.buffer);
        free(checkout_text.buffer);
        free(checkout_success.buffer);
        if (!arg_tag) {
            free(tag.buffer);
        }
        return ec;
    }

    if ((ec = wait_with_animation(p.h_job, p.pi.hProcess, is_console, h_out, checkout_text, checkout_success, checkout_failure)) != SUCCESS) {
        println_error("wait_with_animation failed");
        free(t_info.path.buffer);
        free(checkout_text.buffer);
        free(checkout_success.buffer);
        free(checkout_failure.buffer);
        CloseHandle(p.pi.hProcess);
        CloseHandle(p.pi.hThread);
        if (!arg_tag) {
            free(tag.buffer);
        }
        return ec;
    }
    free(t_info.path.buffer);
    free(checkout_text.buffer);
    free(checkout_success.buffer);
    free(checkout_failure.buffer);
    CloseHandle(p.pi.hProcess);
    CloseHandle(p.pi.hThread);
    if (!arg_tag) {
        free(tag.buffer);
    }

    init_tool_info(&t_info);
    readonly_wstring python_path;
    if ((ec = to_absolute_path(L"comfy-env\\Scripts\\python.exe", &python_path)) != SUCCESS) {
        println_error("to_absolute_path error");
        return ec;
    }
    t_info.path = python_path;

    init_process(&p);
    discard_io(&p);

    if ((ec = execute(&p, &t_info, L"-m pip install -r requirements.txt")) != SUCCESS) {
        println_error("execute failed");
        free(t_info.path.buffer);
        return ec;
    }

    if ((ec = wait_with_animation(p.h_job, p.pi.hProcess, is_console, h_out,
        (readonly_wstring){L"Installing python packages from requirements.txt", ws_len(L"Installing python packages from requirements.txt")},
        (readonly_wstring){L"Successfully installed python packages from requirements.txt", ws_len(L"Successfully installed python packages from requirements.txt")},
        (readonly_wstring){L"Failed to install python packages from requirements.txt", ws_len(L"Failed to install python packages from requirements.txt")})) != SUCCESS) {
        println_error("error during animation");
        CloseHandle(p.pi.hProcess);
        CloseHandle(p.pi.hThread);
        free(t_info.path.buffer);
        return ec;
    }
    CloseHandle(p.pi.hProcess);
    CloseHandle(p.pi.hThread);

    if (new_manager) {
        init_process(&p);
        discard_io(&p);

        if ((ec = execute(&p, &t_info, L"-m pip install -r manager_requirements.txt")) != SUCCESS) {
            println_error("execute failed");
            free(t_info.path.buffer);
            return ec;
        }

        if ((ec = wait_with_animation(p.h_job, p.pi.hProcess, is_console, h_out,
            (readonly_wstring){L"Installing python packages from manager_requirements.txt", ws_len(L"Installing python packages from manager_requirements.txt")},
            (readonly_wstring){L"Successfully installed python packages from manager_requirements.txt", ws_len(L"Successfully installed python packages from manager_requirements.txt")},
            (readonly_wstring){L"Failed to install python packages from manager_requirements.txt", ws_len(L"Failed to install python packages from manager_requirements.txt")})) != SUCCESS) {
            CloseHandle(p.pi.hProcess);
            CloseHandle(p.pi.hThread);
            free(t_info.path.buffer);
            return ec;
            }

        CloseHandle(p.pi.hProcess);
        CloseHandle(p.pi.hThread);
        free(t_info.path.buffer);
    }

    return SUCCESS;
}