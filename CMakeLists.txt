cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)

set(CMAKE_SYSTEM_VERSION 8.1 CACHE STRING "" FORCE)

project(SkyrimOutfitSystemSE CXX)

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
set(SKSE_SUPPORT_XBYAK 1)

find_package(CommonLibSSE REQUIRED)

################################################################################
# MSVC runtime library
################################################################################
string(CONCAT "MSVC_RUNTIME_LIBRARY_STR"
        $<$<CONFIG:Debug>:MultiThreadedDebug>
        $<$<CONFIG:Release>:MultiThreaded>)

################################################################################
# Rust Stuff
################################################################################
include(FetchContent)

FetchContent_Declare(
        Corrosion
        GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
        GIT_TAG v0.2.1 # Optionally specify a commit hash, version tag or branch here
)

FetchContent_MakeAvailable(Corrosion)

corrosion_import_crate(MANIFEST_PATH Cargo.toml)

get_target_property(RE_INCLUDE CommonLibSSE::CommonLibSSE INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(RE_OPTIONS CommonLibSSE::CommonLibSSE INTERFACE_COMPILE_OPTIONS)
get_target_property(RE_DEFINITIONS CommonLibSSE::CommonLibSSE INTERFACE_COMPILE_DEFINITIONS)
set(RUST_HEADER_PATH ${CMAKE_CURRENT_BINARY_DIR}/rust-include/)
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_RUST_MSVC_RUNTIME_LIBRARY "/MT")
else()
    set(CMAKE_RUST_MSVC_RUNTIME_LIBRARY "/MTd")
endif()
corrosion_set_env_vars(sos INCLUDE_PATHS="${RE_INCLUDE}" OPTIONS="${RE_OPTIONS}" DEFINITIONS="${RE_DEFINITIONS}" HEADER_GEN="${RUST_HEADER_PATH}" CRATE_CC_NO_DEFAULTS=1 CXXFLAGS="${CMAKE_RUST_MSVC_RUNTIME_LIBRARY}")

add_library(sos-proxy STATIC ${RUST_HEADER_PATH}/bindings.h src/cpp/dummy.cpp)
add_custom_command(OUTPUT ${RUST_HEADER_PATH}/bindings.h COMMAND ${CMAKE_COMMAND} -E copy_if_different ${RUST_HEADER_PATH}/bindings.h.tmp ${RUST_HEADER_PATH}/bindings.h DEPENDS sos-static)
target_link_libraries(sos-proxy PUBLIC sos-static)
target_include_directories(sos-proxy INTERFACE ${RUST_HEADER_PATH} src/rust/)

################################################################################
# Target
################################################################################
add_library(SkyrimOutfitSystemSE SHARED
    src/cpp/include/version.h
    src/cpp/version.rc
    src/cpp/include/SOS_PCH.h
    src/cpp/main.cpp
    src/cpp/include/OutfitSystem.h
    src/cpp/OutfitSystem.cpp
    src/cpp/include/PlayerSkinning.h
    src/cpp/hooking/Hooks_AE.cpp
    src/cpp/hooking/Hooks_PRE_AE.cpp
    src/cpp/hooking/Patches.cpp
    src/cpp/Utility.cpp
    src/cpp/include/RE/REAugments.h
    src/cpp/RE/REAugments.cpp
    src/cpp/RustSymbols.cpp)

set(ROOT_NAMESPACE SkyrimOutfitSystemSE)

set_target_properties(SkyrimOutfitSystemSE PROPERTIES
    VS_GLOBAL_KEYWORD "Win32Proj"
)

set_target_properties(SkyrimOutfitSystemSE PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
)

target_include_directories(SkyrimOutfitSystemSE PUBLIC
    src/cpp/include/
)

target_precompile_headers(SkyrimOutfitSystemSE PRIVATE
    src/cpp/include/SOS_PCH.h
)

target_compile_definitions(SkyrimOutfitSystemSE PRIVATE
    XBYAK_NO_OP_NAMES
    ENABLE_SKYRIM_SE
    ENABLE_SKYRIM_AE
    ENABLE_COMMONLIBSSE_TESTING
)

target_compile_features(SkyrimOutfitSystemSE PRIVATE
    cxx_std_20
)

if(MSVC)
    target_compile_options(SkyrimOutfitSystemSE PRIVATE
        "/sdl"	    # Enable Additional Security Checks
        "/utf-8"	# Set Source and Executable character sets to UTF-8
        "/Zi"	    # Debug Information Format

        "/EHsc"     # C++ exception handling

        "/Zc:preprocessor"	# Enable preprocessor conformance mode

        "/experimental:external"
        "/external:anglebrackets"
        "/external:W0"

        "/wd4200"   # nonstandard extension used : zero-sized array in struct/union
    )

    target_link_options(SkyrimOutfitSystemSE PRIVATE
        "$<$<CONFIG:DEBUG>:/INCREMENTAL;/OPT:NOREF;/OPT:NOICF>"
        "$<$<CONFIG:RELEASE>:/INCREMENTAL:NO;/OPT:REF;/OPT:ICF;/DEBUG:FULL>"
    )

    set_target_properties(SkyrimOutfitSystemSE PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_RUNTIME_LIBRARY_STR})
endif()

################################################################################
# Compile definitions
################################################################################

################################################################################
# Install steps
################################################################################
add_custom_command(TARGET SkyrimOutfitSystemSE POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:SkyrimOutfitSystemSE> "${CMAKE_CURRENT_SOURCE_DIR}/mod_files/SKSE/Plugins/SkyrimOutfitSystemSE.dll"
)

################################################################################
# Dependencies
################################################################################
# Link with other targets.
find_package(span-lite REQUIRED CONFIG)
find_package(spdlog REQUIRED CONFIG)
find_package(xbyak REQUIRED CONFIG)

target_link_libraries(SkyrimOutfitSystemSE PUBLIC
    CommonLibSSE::CommonLibSSE
    nonstd::span-lite
    spdlog::spdlog
    xbyak::xbyak
    sos-proxy
)

target_include_directories(SkyrimOutfitSystemSE PRIVATE)
