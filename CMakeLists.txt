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

################################################################################
# Find and configure Google Protocol Buffers
################################################################################
find_package(Protobuf REQUIRED)
message(STATUS "Found protoc at ${Protobuf_PROTOC_EXECUTABLE}")

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

add_subdirectory(dependencies/CommonLibSSE)

################################################################################
# Build Protocol Buffers
################################################################################
protobuf_generate_cpp(
    PROTO_SRC PROTO_HDR
    src/protos/outfit.proto
)

add_library(ProtocolBuffers STATIC ${PROTO_SRC} ${PROTO_HDR})

target_link_libraries(ProtocolBuffers PUBLIC protobuf::libprotobuf)

target_include_directories(ProtocolBuffers PUBLIC ${CMAKE_CURRENT_BINARY_DIR}) # The headers are generated right into the output dir.

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
    src/Utility.cpp
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

target_compile_definitions(SkyrimOutfitSystemSE
        PRIVATE
        "SKYRIM_RUNTIME=${SKYRIM_RUNTIME}"
        XBYAK_NO_OP_NAMES)

target_compile_features(
        SkyrimOutfitSystemSE
        PRIVATE
        cxx_std_20
)

if(MSVC)
    target_compile_options(SkyrimOutfitSystemSE PRIVATE
            "/sdl"	    # Enable Additional Security Checks
            "/utf-8"	# Set Source and Executable character sets to UTF-8
            "/Zi"	    # Debug Information Format

            "/permissive-"   	# Standards conformance
            "/Zc:preprocessor"	# Enable preprocessor conformance mode

            "/wd4200"   # nonstandard extension used : zero-sized array in struct/union

            "$<$<CONFIG:DEBUG>:>"
            "$<$<CONFIG:RELEASE>:/Zc:inline;/JMC-;/Ob3>"
            )
    target_link_options(SkyrimOutfitSystemSE PRIVATE
            "$<$<CONFIG:DEBUG>:/INCREMENTAL;/OPT:NOREF;/OPT:NOICF>"
            "$<$<CONFIG:RELEASE>:/INCREMENTAL:NO;/OPT:REF;/OPT:ICF;/DEBUG:FULL>"
            )
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
    CommonLibSSE
    nonstd::span-lite
    spdlog::spdlog
    ProtocolBuffers
    xbyak::xbyak
)
