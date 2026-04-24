/*
install_comfy installs git if needed or else uses the installed git clones ComfyUI from github,
installs python if needed or else uses the installed python creates a venv in the ComfyUI folder and
installs torch packgages for a choses GPU in the venv and installs the requirements.txt
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
#include "utils.h"

// install_comfy [--tag <tag>] [--with-manager] [--new-manager] [--intel, RDNA3, RDNA3.5, RDNA4] [--python <version>] [--path <path>]
int wmain(int argc, wchar_t *argv[]) {
#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
#endif

    if (argc > 10) {
        println_error("too many arguments");
        return TOO_MANY_ARGUMENTS;
    }
    HANDLE h_out;
    bool is_console;
    error_code ec;

    if ((ec = init_console(&h_out, &is_console)) != SUCCESS) {
        return ec;
    }

    readonly_wstring tag = {};
    version python_version = {};
    python_version.major = 3;
    python_version.minor = 13;
    python_version.how_many = 2;
    bool arg_tag = false;
    bool with_manager = false;
    bool intel = false;
    bool rdna3 = false;
    bool rdna35 = false;
    bool rdna4 = false;
    bool python = false;
    bool new_manager = false;
    wchar_t *path = nullptr;

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
            else if (wcscmp(argv[i], L"--with-manager") == 0) {
                if (with_manager) {
                    println_error("--with-manager already set");
                    return INVALID_ARGUMENT;
                }
                with_manager = true;
            }
            else if (wcscmp(argv[i], L"--intel") == 0) {
                if (intel || rdna3 || rdna35 || rdna4) {
                    println_error("gpu already specified");
                    return INVALID_ARGUMENT;
                }
                intel = true;
            }
            else if (wcscmp(argv[i], L"--RDNA3") == 0) {
                if (intel || rdna3 || rdna35 || rdna4) {
                    println_error("gpu already specified");
                    return INVALID_ARGUMENT;
                }
                rdna3 = true;
            }
            else if (wcscmp(argv[i], L"--RDNA3.5") == 0) {
                if (intel || rdna3 || rdna35 || rdna4) {
                    println_error("gpu already specified");
                    return INVALID_ARGUMENT;
                }
                rdna35 = true;
            }
            else if (wcscmp(argv[i], L"--RDNA4") == 0) {
                if (intel || rdna3 || rdna35 || rdna4) {
                    println_error("gpu already specified");
                    return INVALID_ARGUMENT;
                }
                rdna4 = true;
            }
            else if (wcscmp(argv[i], L"--python") == 0) {
                if (python) {
                    println_error("--python version already set");
                    return INVALID_ARGUMENT;
                }
                if (i == argc - 1) {
                    println_error("missing version\nused --python but no version was specified");
                    return INVALID_ARGUMENT;
                }
                int major, minor, chars_read;
                int count = swscanf(argv[++i], L"%d.%d%n", &major, &minor, &chars_read);
                if (wcslen(argv[i]) != chars_read) {
                    println_error("invalid version\nuse: <major>.<minor>");
                    return INVALID_ARGUMENT;
                }
                if (major != 3) {
                    println_error("invalid version\nmajor has to be 3 but got %d", major);
                    return INVALID_ARGUMENT;
                }
                if (minor < 10) {
                    println_error("invalid version\nminor has to be >= 10 but got %d", minor);
                    return INVALID_ARGUMENT;
                }
                if (minor > 14) {
                    println_error("invalid version\nminor has to be <= 14 but got %d", minor);
                    return INVALID_ARGUMENT;
                }
                if (minor < 12) {
                    puts(YELLOW "Warning: ComfyUI recommends using python version 3.12 - 3.14 with 3.13 being the most stable\nEspecially for newer ComfyUI versions older python versions may cause issues" RESET);
                }
                python_version.major = major;
                python_version.minor = minor;
                python_version.how_many = count;
                python = true;
            }
            else if (wcscmp(argv[i], L"--path") == 0) {
                if (i == argc - 1) {
                    println_error("missing path argument");
                    return INVALID_ARGUMENT;
                }
                path = argv[++i];
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

    readonly_wstring full_path = {nullptr, 0};
    if (path) {
        if (wcslen(path) > 200) {
            println_error("Your path is longer than 200 letters, that will very likely cause long paths in the ComfyUI installation (if it is not itself a long path already) and making everything work especially pythons venv and making sure there are only the correct packages installed in the venv at the long path is not worth the effort, just install it at a shorter path");
            // fuck it
            return INVALID_ARGUMENT;  // return FUCK_IT;
        }

        if ((ec = to_absolute_path(path, &full_path)) != SUCCESS) {
            println_error("to_absolute_path error");
            return ec;
        }

        int result = SHCreateDirectoryExW(nullptr, full_path.buffer, nullptr);
        if (result != ERROR_SUCCESS && result != ERROR_ALREADY_EXISTS) {
            println_error("error creating directory: %ls\n", full_path.buffer);
            println_error("error code: %d", result);
            return FAILURE;
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
            free(full_path.buffer);
            return ec;
        }
        if ((ec = search_tool(&t_info)) != SUCCESS) {
            free(full_path.buffer);
            println_error("Git not found");
            return ec;
        }
    }
    else {
        puts(GREEN "Found git" RESET);
    }

    if (!arg_tag) {
        if ((ec = get_latest_tag(&t_info, &p, &tag, h_out)) != SUCCESS) {
            free(full_path.buffer);
            free(t_info.path.buffer);
            return ec;
        }
    }

    init_process(&p);
    discard_io(&p);
    p.working_directory = full_path.buffer;

    wchar_t clone_part1[] = L"clone --branch ";
    size_t len_clone_part1 = ws_len(clone_part1);

    wchar_t clone_part2[] = L" --depth 1 https://github.com/Comfy-Org/ComfyUI.git";
    size_t len_clone_part2 = ws_len(clone_part2);

    readonly_wstring args;
    if ((ec = wstr_cat(len_clone_part1 + len_clone_part2 + tag.len, &args, 3, (readonly_wstring){clone_part1, len_clone_part1}, tag, (readonly_wstring){clone_part2, len_clone_part2})) != SUCCESS) {
        println_error("wstr_cat failed");
        free(tag.buffer);
        free(t_info.path.buffer);
        free(full_path.buffer);
        return ec;
    }

    if ((ec = execute_s(&p, &t_info, args)) != SUCCESS) {
        println_error("execute_s failed");
        free(tag.buffer);
        free(args.buffer);
        free(t_info.path.buffer);
        free(full_path.buffer);
        return ec;
    }
    free(args.buffer);

    wchar_t text_part[] = L"Cloning ComfyUI tag ";
    size_t len_text_part = ws_len(text_part);

    wchar_t success_part[] = L"Successfully cloned ComfyUI tag ";
    size_t len_success_part = ws_len(success_part);

    wchar_t failure_part[] = L"Failed to clone ComfyUI tag ";
    size_t len_failure_part = ws_len(failure_part);

    readonly_wstring text;
    if ((ec = wstr_cat(len_text_part + tag.len, &text, 2, (readonly_wstring){text_part, len_text_part}, tag)) != SUCCESS) {
        println_error("wstr_cat failed");
        if (!arg_tag) {
            free(tag.buffer);
        }
        free(full_path.buffer);
        free(t_info.path.buffer);
        return ec;
    }

    readonly_wstring success;
    if ((ec = wstr_cat(len_success_part + tag.len, &success, 2, (readonly_wstring){success_part, len_success_part}, tag)) != SUCCESS) {
        println_error("wstr_cat failed");
        if (!arg_tag) {
            free(tag.buffer);
        }
        free(t_info.path.buffer);
        free(text.buffer);
        free(full_path.buffer);
        return ec;
    }

    readonly_wstring failure;
    if ((ec = wstr_cat(len_failure_part + tag.len, &failure, 2, (readonly_wstring){failure_part, len_failure_part}, tag)) != SUCCESS) {
        println_error("wstr_cat failed");
        if (!arg_tag) {
            free(tag.buffer);
        }
        free(t_info.path.buffer);
        free(text.buffer);
        free(success.buffer);
        free(full_path.buffer);
        return ec;
    }

    if ((ec = wait_with_animation(p.h_job, p.pi.hProcess, is_console, h_out, text, success, failure)) != SUCCESS) {
        free(text.buffer);
        if (!arg_tag) {
            free(tag.buffer);
        }
        free(t_info.path.buffer);
        free(success.buffer);
        free(failure.buffer);
        free(full_path.buffer);
        return ec;
    }
    free(text.buffer);
    free(success.buffer);
    free(failure.buffer);

    if (!arg_tag) {
        free(tag.buffer);
    }

    CloseHandle(p.pi.hProcess);
    CloseHandle(p.pi.hThread);

    init_process(&p);
    discard_io(&p);

    readonly_wstring comfy = {L"ComfyUI", ws_len(L"ComfyUI")};

    if (path) {
        if ((ec = path_combine(full_path, comfy, &comfy)) != SUCCESS) {
            println_error("path_combine failed");
            free(full_path.buffer);
            return ec;
        }
    }
    readonly_wstring rel_comfy = comfy;
    if ((ec = to_absolute_path(comfy.buffer, &comfy)) != SUCCESS) {
        println_error("to_absolute_path error");
        free(full_path.buffer);
        free(rel_comfy.buffer);
        return ec;
    }
    if (path) {
        free(rel_comfy.buffer);
    }

    readonly_wstring custom_nodes = {L"custom_nodes", ws_len(L"custom_nodes")};

    if (with_manager) {
        if ((ec = path_combine(comfy, custom_nodes, &custom_nodes)) != SUCCESS) {
            free(t_info.path.buffer);
            println_error("path_combine failed");
            return ec;
        }

        p.working_directory = custom_nodes.buffer;

        if ((ec = execute(&p, &t_info, L"clone https://github.com/Comfy-Org/ComfyUI-Manager.git")) != SUCCESS) {
            println_error("execute failed");
            free(custom_nodes.buffer);
            return ec;
        }

        if ((ec = wait_with_animation(p.h_job, p.pi.hProcess, is_console, h_out,
            (readonly_wstring){L"Cloning ComfyUI-Manager", ws_len(L"Cloning ComfyUI-Manager")},
            (readonly_wstring){L"Successfully cloned ComfyUI-Manager", ws_len(L"Successfully cloned ComfyUI-Manager")},
            (readonly_wstring){L"Failed cloning ComfyUI-Manager", ws_len(L"Failed cloning ComfyUI-Manager")})) != SUCCESS) {
            println_error("execute failed");
            free(custom_nodes.buffer);
            free(t_info.path.buffer);
            return ec;
            }
        free(custom_nodes.buffer);
    }

    free(t_info.path.buffer);

    init_tool_info(&t_info);
    t_info.version = python_version;
    t_info.use_version = true;
    t_set_name(&t_info, L"python");
    t_set_sub_key(&t_info, L"SOFTWARE\\Python\\PythonCore\\%s\\InstallPath");

    if ((ec = search_tool(&t_info)) != SUCCESS) {
        if (ec == VERSION_MISMATCH) {
            println_error("Specified python version not found");
        }
        else if (ec == PATH_NOT_FOUND) {
            println_error("Python not found");
        }
        wchar_t python_part[] = L"Installing python ";
        size_t python_part_len = ws_len(python_part);

        readonly_wstring py_version;
        if ((ec = get_version_string(t_info.version, &py_version)) != SUCCESS) {
            println_error("get_version_string failed");
            return ec;
        }

        if ((ec = wstr_cat(python_part_len + py_version.len, &text, 2, (readonly_wstring){python_part, python_part_len}, py_version)) != SUCCESS) {
            println_error("wstr_cat failed");
            free(py_version.buffer);
            return ec;
        }

        if ((ec = install_python(is_console, h_out, text, t_info.version)) != SUCCESS) {
            free(text.buffer);
            free(py_version.buffer);
            return ec;
        }
        free(text.buffer);
        free(py_version.buffer);
        if ((ec = search_tool(&t_info)) != SUCCESS) {
            println_error("Python not found");
            return ec;
        }
    }
    else {
        puts(GREEN "Found python" RESET);
    }

    init_process(&p);
    discard_io(&p);

    p.working_directory = comfy.buffer;

    if ((ec = execute(&p, &t_info, L"-m venv comfy-env")) != SUCCESS) {
        println_error("execute failed");
        free(t_info.path.buffer);
        free(comfy.buffer);
        return ec;
    }

    if ((ec = wait_with_animation(p.h_job, p.pi.hProcess, is_console, h_out,
        (readonly_wstring){L"creating python venv with name: comfy-env", ws_len(L"creating python venv with name: comfy-env")},
        (readonly_wstring){L"Successfully created python venv with name: comfy-env", ws_len(L"Successfully created python venv with name: comfy-env")},
        (readonly_wstring){L"Failed to create python venv with name: comfy-env", ws_len(L"Failed to create python venv with name: comfy-env")})) != SUCCESS) {
        free(t_info.path.buffer);
        CloseHandle(p.pi.hProcess);
        CloseHandle(p.pi.hThread);
        free(comfy.buffer);
        return ec;
        }
    free(t_info.path.buffer);
    CloseHandle(p.pi.hProcess);
    CloseHandle(p.pi.hThread);

    init_tool_info(&t_info);

    readonly_wstring python_rel_path = {L"ComfyUI\\comfy-env\\Scripts\\python.exe", ws_len(L"ComfyUI\\comfy-env\\Scripts\\python.exe")};
    if (path) {
        if ((ec = path_combine(full_path, python_rel_path, &python_rel_path)) != SUCCESS) {
            println_error("path_combine failed");
            return ec;
        }
    }
    free(full_path.buffer);

    readonly_wstring python_path;
    if ((ec = to_absolute_path(python_rel_path.buffer, &python_path)) != SUCCESS) {
        println_error("to_absolute_path error");
        if (path) {
            free(python_rel_path.buffer);
        }
        free(comfy.buffer);
        return ec;
    }
    if (path) {
        free(python_rel_path.buffer);
    }
    t_info.path = python_path;
    init_process(&p);
    p.working_directory = comfy.buffer;
    discard_io(&p);

    readonly_wstring torch = {L"-m pip install torch torchvision torchaudio --extra-index-url https://download.pytorch.org/whl/cu130", ws_len(L"-m pip install torch torchvision torchaudio --extra-index-url https://download.pytorch.org/whl/cu130")};
    readonly_wstring wait_text = {L"installing torch, torchvision and torchaudio Cuda 13.0 python packages", ws_len(L"installing torch, torchvision and torchaudio Cuda 13.0 python packages")};
    readonly_wstring success_text = (readonly_wstring){L_GREEN L"Successfully installed torch, torchvision and torchaudio Cuda 13.0 python packages" L_RESET, ws_len(L_GREEN L"Successfully installed torch, torchvision and torchaudio Cuda 13.0 python packages" L_RESET)};
    readonly_wstring failure_text = (readonly_wstring){L_RED L"Failed to install torch, torchvision and torchaudio Cuda 13.0 python packages" L_RESET, ws_len(L_RED L"Failed to install torch, torchvision and torchaudio Cuda 13.0 python packages" L_RESET)};
    if (intel) {
        torch = (readonly_wstring){L"-m pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/xpu", ws_len(L"-m pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/xpu")};
        wait_text = (readonly_wstring){L"installing torch, torchvision and torchaudio Intel XPU python packages", ws_len(L"installing torch, torchvision and torchaudio Cuda 13.0 python packages")};
        success_text = (readonly_wstring){L_GREEN L"Successfully installed torch, torchvision and torchaudio Intel XPU python packages", ws_len(L_GREEN L"Successfully installed torch, torchvision and torchaudio Cuda 13.0 python packages" L_RESET)};
        failure_text = (readonly_wstring){L_RED L"Failed to install torch, torchvision and torchaudio Intel XPU python packages", ws_len(L_RED L"Failed to install torch, torchvision and torchaudio Cuda 13.0 python packages" L_RESET)};
    }
    else if (rdna3) {
        torch = (readonly_wstring){L"pip install --pre torch torchvision torchaudio --index-url https://rocm.nightlies.amd.com/v2/gfx110X-all", ws_len(L"pip install --pre torch torchvision torchaudio --index-url https://rocm.nightlies.amd.com/v2/gfx110X-all")};
        wait_text = (readonly_wstring){L"installing torch, torchvision and torchaudio RDNA3 python packages", ws_len(L"installing torch, torchvision and torchaudio Cuda 13.0 python packages")};
        success_text = (readonly_wstring){L_GREEN L"Successfully installed torch, torchvision and torchaudio RDNA3 python packages", ws_len(L_GREEN L"Successfully installed torch, torchvision and torchaudio Cuda 13.0 python packages" L_RESET)};
        failure_text = (readonly_wstring){L_RED L"Failed to install torch, torchvision and torchaudio RDNA3 python packages", ws_len(L_RED L"Failed to install torch, torchvision and torchaudio Cuda 13.0 python packages" L_RESET)};
    }
    else if (rdna35) {
        torch = (readonly_wstring){L"pip install --pre torch torchvision torchaudio --index-url https://rocm.nightlies.amd.com/v2/gfx1151/", ws_len(L"pip install --pre torch torchvision torchaudio --index-url https://rocm.nightlies.amd.com/v2/gfx1151/")};
        wait_text = (readonly_wstring){L"installing torch, torchvision and torchaudio RDNA3.5 python packages", ws_len(L"installing torch, torchvision and torchaudio Cuda 13.0 python packages")};
        success_text = (readonly_wstring){L_GREEN L"Successfully installed torch, torchvision and torchaudio RDNA3.5 python packages", ws_len(L_GREEN L"Successfully installed torch, torchvision and torchaudio Cuda 13.0 python packages" L_RESET)};
        failure_text = (readonly_wstring){L_RED L"Failed to install torch, torchvision and torchaudio RDNA3.5 python packages", ws_len(L_RED L"Failed to install torch, torchvision and torchaudio Cuda 13.0 python packages" L_RESET)};
    }
    else if (rdna4) {
        torch = (readonly_wstring){L"pip install --pre torch torchvision torchaudio --index-url https://rocm.nightlies.amd.com/v2/gfx120X-all/", ws_len(L"pip install --pre torch torchvision torchaudio --index-url https://rocm.nightlies.amd.com/v2/gfx120X-all/")};
        wait_text = (readonly_wstring){L"installing torch, torchvision and torchaudio RDNA4 python packages", ws_len(L"installing torch, torchvision and torchaudio Cuda 13.0 python packages")};
        success_text = (readonly_wstring){L_GREEN L"Successfully installed torch, torchvision and torchaudio RDNA4 python packages", ws_len(L_GREEN L"Successfully installed torch, torchvision and torchaudio Cuda 13.0 python packages" L_RESET)};
        failure_text = (readonly_wstring){L_RED L"Failed to install torch, torchvision and torchaudio RDNA4 python packages", ws_len(L_RED L"Failed to install torch, torchvision and torchaudio Cuda 13.0 python packages" L_RESET)};
    }

    if ((ec = execute_s(&p, &t_info, torch)) != SUCCESS) {
        println_error("execute failed");
        free(t_info.path.buffer);
        free(comfy.buffer);
        free(python_path.buffer);
        return ec;
    }

    if ((ec = wait_with_animation(p.h_job, p.pi.hProcess, is_console, h_out, wait_text, success_text, failure_text)) != SUCCESS) {
        CloseHandle(p.pi.hProcess);
        CloseHandle(p.pi.hThread);
        free(t_info.path.buffer);
        free(comfy.buffer);
        free(python_path.buffer);
        return ec;
    }
    CloseHandle(p.pi.hProcess);
    CloseHandle(p.pi.hThread);

    init_process(&p);
    p.working_directory = comfy.buffer;
    discard_io(&p);

    if ((ec = execute(&p, &t_info, L"-m pip install -r requirements.txt")) != SUCCESS) {
        println_error("execute failed");
        free(t_info.path.buffer);
        free(comfy.buffer);
        free(python_path.buffer);
        return ec;
    }

    if ((ec = wait_with_animation(p.h_job, p.pi.hProcess, is_console, h_out,
        (readonly_wstring){L"Installing python packages from requirements.txt", ws_len(L"Installing python packages from requirements.txt")},
        (readonly_wstring){L"Successfully installed python packages from requirements.txt", ws_len(L"Successfully installed python packages from requirements.txt")},
        (readonly_wstring){L"Failed to install python packages from requirements.txt", ws_len(L"Failed to install python packages from requirements.txt")})) != SUCCESS) {
        CloseHandle(p.pi.hProcess);
        CloseHandle(p.pi.hThread);
        free(t_info.path.buffer);
        free(comfy.buffer);
        free(python_path.buffer);
        return ec;
        }

    CloseHandle(p.pi.hProcess);
    CloseHandle(p.pi.hThread);

    if (new_manager) {
        init_process(&p);
        p.working_directory = comfy.buffer;
        discard_io(&p);

        if ((ec = execute(&p, &t_info, L"-m pip install -r manager_requirements.txt")) != SUCCESS) {
            println_error("execute failed");
            free(t_info.path.buffer);
            free(comfy.buffer);
            free(python_path.buffer);
            return ec;
        }

        if ((ec = wait_with_animation(p.h_job, p.pi.hProcess, is_console, h_out,
            (readonly_wstring){L"Installing python packages from manager_requirements.txt", ws_len(L"Installing python packages from manager_requirements.txt")},
            (readonly_wstring){L"Successfully installed python packages from manager_requirements.txt", ws_len(L"Successfully installed python packages from manager_requirements.txt")},
            (readonly_wstring){L"Failed to install python packages from manager_requirements.txt", ws_len(L"Failed to install python packages from manager_requirements.txt")})) != SUCCESS) {
            CloseHandle(p.pi.hProcess);
            CloseHandle(p.pi.hThread);
            free(t_info.path.buffer);
            free(comfy.buffer);
            free(python_path.buffer);
            return ec;
        }

        CloseHandle(p.pi.hProcess);
        CloseHandle(p.pi.hThread);
        free(python_path.buffer);
        free(comfy.buffer);
    }

    free(comfy.buffer);
    free(python_path.buffer);

    return 0;
}