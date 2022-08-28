# Skyrim Outfit System SE Revived

[![Build status](https://ci.appveyor.com/api/projects/status/oxovhnk16gfn9ef3/branch/master?svg=true)](https://ci.appveyor.com/project/thekineticeffect/skyrimoutfitsystemse-mpt1j/branch/master)

This mod is a resurrection of [aers's port](https://github.com/aers/SkyrimOutfitSystemSE) of David J Cobb's [Skyrim Outfit System](https://github.com/DavidJCobb/skyrim-outfit-system) for Skyrim SE, now able to run on newer Skyrim SE versions including Anniversary Edition.

This version also features a better Quickselect system using UIExtensions menus and is also version independent from the SKSE/Skyrim runtime version (except for pre-AE vs AE) by use of the [Address Library](https://www.nexusmods.com/skyrimspecialedition/mods/32444).

# Compatibility

This mod supports AE and pre-AE with different DLLs.

The pre-AE DLL supports runtimes 1.5.73 to 1.5.97.

The AE DLL supports runtimes 1.6.317 to 1.6.353 (so far, it is only tested on 1.6.353).

# License

This mod is derived from aers's SSE port of DavidJCobb's Skyrim Outfit System. Since DavidJCobb originally licensed Skyrim Outfit System under 
CC-BY-NC-SA-4.0, which must be applied to derivative works, this rework also uses the same license.

Some files in the `dependencies` folder are from other places, and fall under their respective licenses.

# Building

## Prerequisites

Before attempting to build this project, please have the following tools installed.

 * Visual Studio 2022 with the C++ Toolchain, including MSVC and CMake
 * [vcpkg](https://github.com/microsoft/vcpkg)
 * [Pyro](https://wiki.fireundubh.com/pyro)
 * [ninja](https://ninja-build.org/)
 * git

## Build Steps

### Preparation

#### Submodules

First, make sure this project is cloned and that the submodules are initialized and updated:

    git submodule update --init

This will initialize various submodules that contain the fork of CommonLibSSE that this mod uses.

#### SKSE (Optional)

If you want to edit and compile Papyrus scripts, you will need to download SKSE. If you only plan to work on the DLL itself, you can skip this step.

You must also manually download SKSE64 2.0.15. Extract it into the `dependencies/skse64` subfolder. You want it such that `skse64_readme.txt` is directly inside of `dependencies/skse64`.

Make sure you have an installation of the exact version of `vcpkg` specified above bootstrapped and ready to go. The version of `vcpkg` determines the versions of the dependencies that will be provided by it.

### Building the C++ Plugin

You will use CMake to build this project. This project is 64-bit statically linked and uses `vcpkg` to get obtain library dependencies. 

Make sure to activate the x86-64 build tools as as follows:

    "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

First, create a build folder (anywhere is fine). For this example, we will create a folder called `build` inside the root of the project.

    mkdir build
    cd build

Now invoke CMake as follows:

    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=${PATH TO VCPKG TOOLCHAIN} -DVCPKG_TARGET_TRIPLET=x64-windows-static -DSKYRIM_VERSION=AE ../

Note that `${PATH TO VCPKG TOOLCHAIN}` is to be replaced with the path to the `vkpkg` toolchain, as described by the [vcpkg documentation](https://vcpkg.io/en/getting-started.html).

> **NOTE:** This command may fail the first time, saying that "protoc.exe" could not be found. If this happens, rerun the cmake command above.

> **WARNING:** `Ninja` is the only supported generator. MSBuild may or may not work.

Take special note of the `-DSKYRIM_VERSION=AE` option. **This mod uses different DLLs for AE and pre-AE**. If you want to build the DLL for AE, use `AE` as shown. If you want to build for pre-AE, use `PRE_AE` instead. If you plan to contribute changes to this mod, they need to work with both variants.

Once the project is successfully configured, build it by running

    cmake --build ./

In a few moments, you should have `SkyrimOutfitSystemSE.dll` in the `mod_files` folder.

### Building the Papyrus code

This project uses Pyro to build the Papyrus scripts. Pyro uses the `skyrimse.ppj` file in the root of this project. I personally use the [VS Code plugin](https://marketplace.visualstudio.com/items?itemName=joelday.papyrus-lang-vscode) for this, but I plan to add instructions on doing it manually.

## Assembling the Mod Package

The CMake and Pyro build process both deposit their build artifacts into the `mod_files` folder. This folder also contains the translation file and `.esp` file and, when both the Papyrus and C++ plugins are built, is the complete mod package.
