# Skyrim Outfit System SE

Port of the SKSE dll portion of David J Cobb's [Skyrim Outfit System](https://github.com/DavidJCobb/skyrim-outfit-system) to SKSE64/Skyrim SE.

The majority of code in this repository is a direct copy of Cobb's; only the patches have significant changes. There are minor changes elsewhere.

# Building
This project is 64-bit statically linked and uses vcpkg to get dependencies. Make sure to run the CMake configure with the following flags.

    -DCMAKE_TOOLCHAIN_FILE=${PATH TO VCPKG TOOLCHAIN} -DVCPKG_TARGET_TRIPLET=x64-windows-static