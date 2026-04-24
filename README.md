Automation tools to install, update and start ComfyUI. For Windows 10 and Windows 11 only. The tool will install python and git for you if it is not installed, to use this feature winget has to be installed, which should be the case on windows 10 and windows 11, but if it is not installed AND you do not have python and git installed the program will fail and abort. By default install_comfy will install the Cuda 13 torch, torchvision and torchaudio packages in the python venv, for this you do not need to use an cli argument. It will install the programs for the user, winget does not need elevation for that and the python installer does also not need it, but if you do not have git installed the git installer will trigger the UAC prompt for elevation, note that only the git installer will get elevated rights through this, not the install_comfy process, you will see that the UAC prompt is for GitForWindows.

# Usage

## install_comfy

```
install_comfy [--tag <tag>] [--with-manager] [--new-manager] [--intel, --RDNA3, --RDNA3.5, --RDNA4] [--python <version>] [--path <path>]
```

Arguments inbetween \[\] are optional, arguments in between <> are mandatory and have to be replaced with a concrete value. Arguments inbetween \[\] seperated with comas mean that you have to choose **one** of them, if you use this optional argument.

This program will fail and abort if at the path where you want to install ComfyUI, already exists a folder named "ComfyUI".

### Explanation

--tag \<tag\>
With this argument you can specify which tag of the ComfyUI repository you want to install. It will make a shallow clone of only this tag. Example: --tag v0.12.3
If you do not use this argument it will clone the newest tag.

--with-manager
It will clone the [ComfyUI-Manager](https://github.com/Comfy-Org/ComfyUI-Manager) into your custom_nodes folder. If you do not set this flag it will not clone the ComfyUI-Manager.

--new-manager
Newer Versions of ComfyUI have a new built in Manager, when you want to use this and use one of these versions, you have to use this flag, it will install the requirements for the new manager in the python venv. If you use this flag but the version you use does not support the new manager, the program will fail and abort.

--python \<version\>
It will use or install the specified python version. You can only specify major and minor version not patch. The only possible options are python 3.10 - 3.14. If you do not use this argument it will use or install python 3.13. Example: --python 3.10

--path \<path\>
You can specify an absolute or a relative path where ComfyUI should be installed. It will create the path if it does not exist. Paths that contain spaces need to be put inbetween "". Note that it creates a folder named "ComfyUI" at this path, which contains all files of the specified or newest tag of the ComfyUI repository.
Examples:
--path hello\world\test42
Then your ComfyUI will be at \<path to install_comfy.exe\>\\hello\\world\\test42\ComfyUI

--path C:Users\\Username\\Documents
Then your ComfyUI will be at C:\\Users\\Username\\Documents\\ComfyUI

--path "C:Users\\Username\\Documents\\folder with spaces"
Then your ComfyUI will be at C:\\Users\\Username\\Documents\\ComfyUI\\folder with spaces

By default install_comfy will install the Cuda 13 torch, torchvision and torchaudio packages in the python venv, so there is no cli argument for it, it is the default, the flags are only for when you want to use other packages so you can either use none of them, then Cuda 13 will be used or one of them. You can not use multiple of them (it is case sensitive you have to write the args exactly like i specified here):

--intel
It will install the Intel XPU torch, torchvision and torchaudio packages in the python venv.

--RDNA3
It will install the AMD RDNA3 torch, torchvision and torchaudio packages in the python venv.

--RDNA3.5
It will install the AMD RDNA3.5 torch, torchvision and torchaudio packages in the python venv.

--RDNA4
It will install the AMD RDNA4 torch, torchvision and torchaudio packages in the python venv.

## update

This program has to be directly in the main "ComfyUI" folder. Otherwise it will not work. Also The python venv "comfy-env", which install_comfy creates needs to exist, so if you use it on an ComfyUI install that was not created by install_comfy you need to crate a venv called "comfy-env".

```
update [--tag <tag>] [--new-manager]
```

Arguments inbetween \[\] are optional, arguments in between <> are mandatory and have to be filled with a concrete value.

### Explanation

--tag \<tag\>
With this argument you can specify which tag of the ComfyUI repository you want to fetch and checkout to. Note that even if this program is called "update" you can also use it to downgrade your ComfyUI install to an old version. It will make a shallow clone of only this tag. Example: --tag v0.12.3
If you do not use this argument it will fetch and checkout to the newest tag.

--new-manager
When this flag is set it will install the new requirements for the new built in manager in the python venv. When you set this flag but do not use a ComfyUI version that has the new manager the program will fail and abort.

## start

This program has to be directly in the main "ComfyUI" folder. Otherwise it will not work. Also The python venv "comfy-env", which install_comfy creates needs to exist, so if you use it on an ComfyUI install that was not created by install_comfy you need to crate a venv called "comfy-env". The args.ini file also need to be directly  in the main "ComfyUI" folder along with this program. When you stop ComfyUI and want to restart make sure to close the Browser tab first, otherwise the browser will occupy the port, causing ComfyUI to fail to start.

### args.ini

in this file you can specify the cli args for the python.exe which will be used on main.py. Note that you should not give main.py as an argument here that will be prepended automatically to the cli arguments, so even when you specify no arguments here python.exe will get the main.py argument.

The file has to look like this:

```
[Arguments]
cli_args="<your arguments>"
```

replace \<your arguments\> with you actual cli argument for ComfyUI which you can find [here](https://github.com/Comfy-Org/ComfyUI/blob/master/comfy/cli_args.py).
If you use a new ComfyUI version wich already has the dynamic vram feature i heavily recommend to use --disable-dynamic-vram since the dynamic vram heavily slows generation down. Also new ComfyUI version have a new built in Manager, to use it you have to put --enable-manager in the cli_args, along with using --new-manager on install_comfy.exe.

Example:

```
[Arguments]
cli_args="--port 8188 --use-pytorch-cross-attention --disable-dynamic-vram --enable-manager"
```

Note that 8188 is the default port, you can also omit this argument.

# Build

You need a C compiler that supports C23 and the Windows SDK to be able to compile the tools. My recommendation is using clang with MSVC toolchain. I recommend to install full [LLVM](https://github.com/llvm/llvm-project/releases/tag/llvmorg-22.1.3) (LLVM-22.1.3-win64.exe). Use the [Visual Studio Installer](https://visualstudio.microsoft.com/de/downloads/) (Community Edition) to install MSVC and the Windows SDK. Note you do not need to install Visual Studio, just use the installer to install MSVC and the Windows SDK. I will only give a tutorial for full LLVM with MSVC toolchain + Windows SDK installation. Note that you can not compile the tools with MSVC since MSVC does not support C23.

compile the ressource

```
llvm-rc ressource.rc
```

compile install_comfy

```
clang -O3 -march=native -DNDEBUG -std=c23 -flto -fuse-ld=lld-link -lkernel32 -lshell32 -ladvapi32 -lversion -lwinmm -lshlwapi -lpathcch install_comfy.c path_utils.c process_utils.c utils.c ressource.res -o install_comfy.exe
```

compile update

```
clang -O3 -march=native -DNDEBUG -std=c23 -flto -fuse-ld=lld-link -lkernel32 -lshell32 -ladvapi32 -lversion -lwinmm -lshlwapi -lpathcch update.c path_utils.c process_utils.c utils.c ressource.res -o update.exe
```

compile start

```
clang -O3 -march=native -DNDEBUG -std=c23 -flto -fuse-ld=lld-link -lkernel32 -lshell32 -ladvapi32 -lversion -lwinmm -lshlwapi -lpathcch -lws2_32 start.c path_utils.c process_utils.c utils.c ressource.res -o start.exe
```
