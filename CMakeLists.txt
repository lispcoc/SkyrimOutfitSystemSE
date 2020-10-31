cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)

set(CMAKE_SYSTEM_VERSION 8.1 CACHE STRING "" FORCE)

project(SkyrimOutfitSystemSE CXX)

set(SKYRIM_RUNTIME 1_5_73)

if(${SKYRIM_RUNTIME} STREQUAL "1_5_97")
    set(SKSE_RUNTIME_VERSION 0x01050610)
elseif(${SKYRIM_RUNTIME} STREQUAL "1_5_73")
    set(SKSE_RUNTIME_VERSION 0x01050490)
endif()

################################################################################
# Set target arch type if empty. Visual studio solution generator provides it.
################################################################################
if(NOT CMAKE_CL_64)
    message(FATAL_ERROR "Must use a 64-bit toolchain!")
endif()

if(NOT "${VCPKG_TARGET_TRIPLET}" STREQUAL "x64-windows-static")
    message(FATAL_ERROR "Must set VCPKG_TARGET_TRIPLET to x64-windows-static! It is currently ${VCPKG_TARGET_TRIPLET}")
endif()

################################################################################
# Global configuration types
################################################################################
set(CMAKE_CONFIGURATION_TYPES
    "Debug"
    "Release"
    CACHE STRING "" FORCE
)

################################################################################
# Global compiler options
################################################################################
if(MSVC)
    # remove default flags provided with CMake for MSVC
    set(CMAKE_CXX_FLAGS "")
    set(CMAKE_CXX_FLAGS_DEBUG "")
    set(CMAKE_CXX_FLAGS_RELEASE "")
endif()

set(DEFAULT_CXX_EXCEPTION_HANDLING "/EHsc")

################################################################################
# Global linker options
################################################################################
if(MSVC)
    # remove default flags provided with CMake for MSVC
    set(CMAKE_EXE_LINKER_FLAGS "")
    set(CMAKE_MODULE_LINKER_FLAGS "")
    set(CMAKE_SHARED_LINKER_FLAGS "")
    set(CMAKE_STATIC_LINKER_FLAGS "")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_DEBUG "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_DEBUG "${CMAKE_STATIC_LINKER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS}")
endif()

################################################################################
# Nuget packages function stub.
################################################################################
function(use_package TARGET PACKAGE VERSION)
    message(WARNING "No implementation of use_package. Create yours. "
                    "Package \"${PACKAGE}\" with version \"${VERSION}\" "
                    "for target \"${TARGET}\" is ignored!")
endfunction()

################################################################################
# Use solution folders feature
################################################################################
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

################################################################################
# Sub-projects
################################################################################
add_subdirectory(dependencies/CommonLibSSE)
add_subdirectory(dependencies)

################################################################################
# Target
################################################################################
add_library(SkyrimOutfitSystemSE SHARED
    include/version.h
    src/version.rc
    include/SOS_PCH.h
    src/main.cpp
    include/cobb/strings.h
    include/cobb/utf8naturalsort.h
    src/cobb/utf8naturalsort.cpp
    include/cobb/utf8string.h
    src/cobb/utf8string.cpp
    include/ArmorAddonOverrideService.h
    src/ArmorAddonOverrideService.cpp
    include/OutfitSystem.h
    src/OutfitSystem.cpp
    include/PlayerSkinning.h
    src/PlayerSkinning.cpp
    include/StorableObject.h
    src/StorableObject.cpp
)

set(ROOT_NAMESPACE SkyrimOutfitSystemSE)

set_target_properties(SkyrimOutfitSystemSE PROPERTIES
    VS_GLOBAL_KEYWORD "Win32Proj"
)

set_target_properties(SkyrimOutfitSystemSE PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
)

target_include_directories(SkyrimOutfitSystemSE PUBLIC
    include/
)

target_precompile_headers(SkyrimOutfitSystemSE PRIVATE
    include/SOS_PCH.h
)

################################################################################
# MSVC runtime library
################################################################################
get_property(MSVC_RUNTIME_LIBRARY_DEFAULT TARGET SkyrimOutfitSystemSE PROPERTY MSVC_RUNTIME_LIBRARY)

string(CONCAT "MSVC_RUNTIME_LIBRARY_STR"
    $<$<CONFIG:Debug>:MultiThreadedDebug>
    $<$<CONFIG:Release>:MultiThreaded>
    $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>>:${MSVC_RUNTIME_LIBRARY_DEFAULT}>
)

set_target_properties(SkyrimOutfitSystemSE PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_RUNTIME_LIBRARY_STR})

# Also force it on CommonLibSSE
set_target_properties(CommonLibSSE PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_RUNTIME_LIBRARY_STR})

################################################################################
# Compile definitions
################################################################################
target_compile_definitions(SkyrimOutfitSystemSE PRIVATE
    "SKYRIM_RUNTIME=${SKYRIM_RUNTIME}"
)

################################################################################
# Compile and link options
################################################################################
if(MSVC)
    target_compile_options(SkyrimOutfitSystemSE PRIVATE
        $<$<CONFIG:Debug>:
            /GL-;
            /Od;
            /RTC1;
            /fp:except;
            /guard:cf;
            /Y-
        >
        $<$<CONFIG:Release>:
            /GL;
            /O2;
            /Oi
        >
        /MP;
        /std:c++17;
        /sdl-;
        /Gy;
        /W3;
        /Zi;
        /TP;
        /experimental:external;
        /external:anglebrackets;
        /external:W0;
        /Zc:alignedNew;
        /Zc:auto;
        /Zc:char8_t;
        /Zc:__cplusplus;
        /Zc:externC;
        /Zc:externConstexpr;
        /Zc:forScope;
        /Zc:hiddenFriend;
        /Zc:implicitNoexcept;
        /Zc:lambda;
        /Zc:noexceptTypes;
        /Zc:preprocessor;
        /Zc:referenceBinding;
        /Zc:rvalueCast;
        /Zc:sizedDealloc;
        /Zc:strictStrings;
        /Zc:ternary;
        /Zc:threadSafeInit;
        /Zc:tlsGuards;
        /Zc:trigraphs;
        /Zc:wchar_t;
        /Zc:twoPhase-;
        ${DEFAULT_CXX_EXCEPTION_HANDLING}
    )
    target_link_options(SkyrimOutfitSystemSE PRIVATE
        $<$<CONFIG:Debug>:
            /INCREMENTAL
        >
        $<$<CONFIG:Release>:
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO
        >
        /DEBUG;
        /SUBSYSTEM:WINDOWS
    )
endif()


################################################################################
# Dependencies
################################################################################
# Link with other targets.
find_package(span-lite REQUIRED CONFIG)
find_package(spdlog REQUIRED CONFIG)

target_link_libraries(SkyrimOutfitSystemSE PUBLIC
    CommonLibSSE
    skse64
    skse64_common
    nonstd::span-lite
    spdlog::spdlog
)

