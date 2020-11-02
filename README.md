# Skyrim Outfit System SE Revived

This mod is a resurrection of [aers's port](https://github.com/aers/SkyrimOutfitSystemSE) of David J Cobb's [Skyrim Outfit System](https://github.com/DavidJCobb/skyrim-outfit-system) for Skyrim SE.

This version now features a better Quickselect system using UIExtensions menus and is also version independent from the SKSE/Skyrim runtimes version by use of the [Address Library](https://www.nexusmods.com/skyrimspecialedition/mods/32444). It has been tested on Skyrim runtimes 1.5.73 to 1.5.97.

# License

This mod is derived from aers's SSE port of DavidJCobb's Skyrim Outfit System. Since DavidJCobb originally licensed Skyrim Outfit System under 
CC-BY-NC-SA-4.0, which must be applied to derivative works, this rework also uses the same license.

Some files in the `dependencies` folder are from other places, and fall under their respective licenses.

# Building

More instructions will be added here soon.

This project is 64-bit statically linked and uses vcpkg to get dependencies. Make sure to run the CMake configure with the following flags.

    -DCMAKE_TOOLCHAIN_FILE=${PATH TO VCPKG TOOLCHAIN} -DVCPKG_TARGET_TRIPLET=x64-windows-static