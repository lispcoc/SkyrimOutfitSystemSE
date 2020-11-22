# Check that we have the correct SKSE
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/skse64/skse64_${SKYRIM_RUNTIME}.dll")
else()
    message(FATAL_ERROR "Could not find SKSE for Skyrim version ${SKYRIM_RUNTIME}! Make sure you have skse64_${SKYRIM_RUNTIME}.dll")
endif()

################################################################################
################################################################################
## relocation patch
################################################################################
################################################################################
# Perform the relocation patch processing
add_custom_command(
        OUTPUT
        ${CMAKE_SOURCE_DIR}/dependencies/skse64/src/skse64/skse64_common/addrlib_offsets.h
        ${CMAKE_SOURCE_DIR}/dependencies/skse64/src/skse64/skse64_common/lookup.h
        ${CMAKE_SOURCE_DIR}/dependencies/skse64/src/skse64/skse64_common/lookup.cpp
        ${CMAKE_SOURCE_DIR}/dependencies/skse64/src/skse64/skse64_common/versiondb.h
        ${CMAKE_SOURCE_DIR}/dependencies/skse64/src/skse64/skse64_common/eternal.hpp
        ${CMAKE_SOURCE_DIR}/dependencies/skse64/src/skse64/skse64_common/Relocation.cpp
        ${CMAKE_SOURCE_DIR}/dependencies/skse64/src/skse64/skse64_common/Relocation.h
        ${CMAKE_SOURCE_DIR}/dependencies/skse64/src/skse64/skse64_common/Utilities.h

        COMMAND $<TARGET_FILE:addrheader> ${SKYRIM_RUNTIME} ${CMAKE_SOURCE_DIR}/dependencies/AddressLibrary/ ${CMAKE_CURRENT_BINARY_DIR}/ ${CMAKE_SOURCE_DIR}/dependencies//skse64/src/

        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/offsets_${SKYRIM_RUNTIME}.h ${CMAKE_SOURCE_DIR}/dependencies/skse64/src/skse64/skse64_common/addrlib_offsets.h
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/src/addrlib/skse_swap/lookup.h ${CMAKE_SOURCE_DIR}/dependencies/skse64/src/skse64/skse64_common/lookup.h
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/src/addrlib/skse_swap/lookup.cpp ${CMAKE_SOURCE_DIR}/dependencies/skse64/src/skse64/skse64_common/lookup.cpp
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/dependencies/AddressLibrary/versiondb.h ${CMAKE_SOURCE_DIR}/dependencies/skse64/src/skse64/skse64_common/versiondb.h
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/dependencies/mapbox_eternal/include/mapbox/eternal.hpp ${CMAKE_SOURCE_DIR}/dependencies/skse64/src/skse64/skse64_common/eternal.hpp

        # TODO: We are not fixing up the NiRTTI files. If you ever need to use those, you'll need to add them here too...
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/src/addrlib/skse_swap/Relocation.cpp ${CMAKE_SOURCE_DIR}/dependencies/skse64/src/skse64/skse64_common/Relocation.cpp
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/src/addrlib/skse_swap/Relocation.h ${CMAKE_SOURCE_DIR}/dependencies/skse64/src/skse64/skse64_common/Relocation.h
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/src/addrlib/skse_swap/Utilities.h ${CMAKE_SOURCE_DIR}/dependencies/skse64/src/skse64/skse64_common/Utilities.h

        DEPENDS addrheader ${CMAKE_SOURCE_DIR}/src/addrlib/skse_swap/Relocation.cpp ${CMAKE_SOURCE_DIR}/src/addrlib/skse_swap/Relocation.h ${CMAKE_SOURCE_DIR}/src/addrlib/skse_swap/Utilities.h
        ${CMAKE_SOURCE_DIR}/dependencies/AddressLibrary/versiondb.h ${CMAKE_SOURCE_DIR}/dependencies/mapbox_eternal/include/mapbox/eternal.hpp
        ${CMAKE_SOURCE_DIR}/src/addrlib/skse_swap/lookup.h ${CMAKE_SOURCE_DIR}/src/addrlib/skse_swap/lookup.cpp
)

################################################################################
################################################################################
## common_vc14
################################################################################
################################################################################
set(PROJECT_NAME common_vc14)

################################################################################
# Source groups
################################################################################
file(GLOB HEADER_FILES ./skse64/src/common/*.h)
file(GLOB SRC_FILES ./skse64/src/common/*.cpp)
set(ALL_FILES ${HEADER_FILES} ${SRC_FILES})

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} STATIC ${ALL_FILES})

set(ROOT_NAMESPACE common_vc14)

set_target_properties(${PROJECT_NAME} PROPERTIES
        VS_GLOBAL_KEYWORD "Win32Proj"
        )
################################################################################
# Output directory
################################################################################
set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_DIRECTORY_DEBUG   "${CMAKE_SOURCE_DIR}/${CMAKE_VS_PLATFORM_NAME}_$ENV{PlatformToolset}/$<CONFIG>/"
        OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/${CMAKE_VS_PLATFORM_NAME}_$ENV{PlatformToolset}/$<CONFIG>/"
        )
set_target_properties(${PROJECT_NAME} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
        )
################################################################################
# MSVC runtime library
################################################################################
get_property(MSVC_RUNTIME_LIBRARY_DEFAULT TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY)
string(CONCAT "MSVC_RUNTIME_LIBRARY_STR"
        $<$<CONFIG:Debug>:
        MultiThreadedDebug
        >
        $<$<CONFIG:Release>:
        MultiThreaded
        >
        $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>>:${MSVC_RUNTIME_LIBRARY_DEFAULT}>
        )
set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_RUNTIME_LIBRARY_STR})

################################################################################
# Include directories
################################################################################
target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/skse64/src/"
        "${CMAKE_CURRENT_SOURCE_DIR}/skse64/src/skse64"
        )

target_compile_definitions(${PROJECT_NAME} PRIVATE PCH_COMMON_ONLY)

target_precompile_headers(${PROJECT_NAME} PRIVATE
        ../include/SKSE_PCH.h
        )

################################################################################
# Compile definitions
################################################################################
target_compile_definitions(${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Debug>:"
        "_DEBUG"
        ">"
        "$<$<CONFIG:Release>:"
        "NDEBUG"
        ">"
        "_LIB;"
        "_MBCS"
        )

################################################################################
# Compile and link options
################################################################################
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:
            /Od
            >
            $<$<CONFIG:Release>:
            /O2;
            /Oi;
            /Gy
            >
            /MP;
            /W3;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            ${DEFAULT_CXX_EXCEPTION_HANDLING};
            /Y-
            )
    target_link_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Release>:
            /OPT:REF;
            /OPT:ICF
            >
            /SUBSYSTEM:WINDOWS
            )
endif()

################################################################################
################################################################################
## skse64_common
################################################################################
################################################################################
set(PROJECT_NAME skse64_common)

################################################################################
# Source groups
################################################################################
file(GLOB HEADER_FILES ./skse64/src/skse64/skse64_common/*.h)
file(GLOB SRC_FILES ./skse64/src/skse64/skse64_common/*.cpp)
set(EXTRA_FILES
        ${CMAKE_SOURCE_DIR}/dependencies/skse64/src/skse64/skse64_common/addrlib_offsets.h
        ${CMAKE_SOURCE_DIR}/dependencies/skse64/src/skse64/skse64_common/lookup.h
        ${CMAKE_SOURCE_DIR}/dependencies/skse64/src/skse64/skse64_common/lookup.cpp
        ${CMAKE_SOURCE_DIR}/dependencies/skse64/src/skse64/skse64_common/versiondb.h
        ${CMAKE_SOURCE_DIR}/dependencies/skse64/src/skse64/skse64_common/eternal.hpp
        ${CMAKE_SOURCE_DIR}/dependencies/skse64/src/skse64/skse64_common/Relocation.cpp
        ${CMAKE_SOURCE_DIR}/dependencies/skse64/src/skse64/skse64_common/Relocation.h
        ${CMAKE_SOURCE_DIR}/dependencies/skse64/src/skse64/skse64_common/Utilities.h)
set(ALL_FILES ${HEADER_FILES} ${SRC_FILES} ${EXTRA_FILES})

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} STATIC ${ALL_FILES})

set(ROOT_NAMESPACE skse64_common)

set_target_properties(${PROJECT_NAME} PROPERTIES
        VS_GLOBAL_KEYWORD "Win32Proj"
        )
set_target_properties(${PROJECT_NAME} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
        )
################################################################################
# MSVC runtime library
################################################################################
get_property(MSVC_RUNTIME_LIBRARY_DEFAULT TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY)
string(CONCAT "MSVC_RUNTIME_LIBRARY_STR"
        $<$<CONFIG:Debug>:
        MultiThreadedDebug
        >
        $<$<CONFIG:Release>:
        MultiThreaded
        >
        $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>>:${MSVC_RUNTIME_LIBRARY_DEFAULT}>
        )
set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_RUNTIME_LIBRARY_STR})

################################################################################
# Include directories
################################################################################
target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/skse64/src/"
        "${CMAKE_CURRENT_SOURCE_DIR}/skse64/src/skse64/"
        )

target_precompile_headers(${PROJECT_NAME} PRIVATE
        ../include/SKSE_PCH.h
        )

################################################################################
# Compile definitions
################################################################################
target_compile_definitions(${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Debug>:"
        "_DEBUG"
        ">"
        "$<$<CONFIG:Release>:"
        "NDEBUG"
        ">"
        "_LIB;"
        "_MBCS"
        )

################################################################################
# Compile and link options
################################################################################
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:
            /Od
            >
            $<$<CONFIG:Release>:
            /O2;
            /Oi;
            /Gy
            >
            /MP;
            /W3;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            ${DEFAULT_CXX_EXCEPTION_HANDLING};
            /Y-
            )
    target_link_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Release>:
            /OPT:REF;
            /OPT:ICF
            >
            /SUBSYSTEM:WINDOWS
            )
endif()

################################################################################
# Dependencies
################################################################################
add_dependencies(${PROJECT_NAME}
        common_vc14
)

# Link with other targets.
target_link_libraries(${PROJECT_NAME} PUBLIC
        common_vc14
)

################################################################################
################################################################################
## skse64
################################################################################
################################################################################

set(PROJECT_NAME skse64)

################################################################################
# Source groups
################################################################################
file(GLOB HEADER_FILES ./skse64/src/skse64/skse64/*.h)
file(GLOB SRC_FILES ./skse64/src/skse64/skse64/*.cpp)
set(ALL_FILES ${HEADER_FILES} ${SRC_FILES})

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} STATIC ${ALL_FILES})

################################################################################
# Includes for CMake from *.props
################################################################################
set(ROOT_NAMESPACE skse64)

set_target_properties(${PROJECT_NAME} PROPERTIES
        VS_GLOBAL_KEYWORD "Win32Proj"
        )
set_target_properties(${PROJECT_NAME} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
        )
################################################################################
# MSVC runtime library
################################################################################
get_property(MSVC_RUNTIME_LIBRARY_DEFAULT TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY)
string(CONCAT "MSVC_RUNTIME_LIBRARY_STR"
        $<$<CONFIG:Debug>:
        MultiThreadedDebug
        >
        $<$<CONFIG:Release>:
        MultiThreaded
        >
        $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>>:${MSVC_RUNTIME_LIBRARY_DEFAULT}>
        )
set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_RUNTIME_LIBRARY_STR})

################################################################################
# Include directories
################################################################################
target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/skse64/src/skse64/"
        "${CMAKE_CURRENT_SOURCE_DIR}/skse64/src/"
        )

target_precompile_headers(${PROJECT_NAME} PRIVATE
        ../include/SKSE_PCH.h
        )

################################################################################
# Compile definitions
################################################################################
target_compile_definitions(${PROJECT_NAME} PUBLIC
        "$<$<CONFIG:Debug>:"
        "_DEBUG"
        ">"
        "$<$<CONFIG:Release>:"
        "NDEBUG"
        ">"
        "_MBCS"
        "_WINDOWS"
        "_USRDLL"
        "SKSE64_EXPORTS"
        "RUNTIME_VERSION=${SKSE_RUNTIME_VERSION}"
        "RUNTIME"
        )

################################################################################
# Compile and link options
################################################################################
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:
            /Od
            >
            $<$<CONFIG:Release>:
            /O2;
            /Oi;
            /Gy
            >
            /MP;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            ${DEFAULT_CXX_EXCEPTION_HANDLING};
            /Y-
            )
    target_link_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Release>:
            /OPT:REF;
            /OPT:ICF
            >
            /SUBSYSTEM:WINDOWS
            )
endif()

################################################################################
# Dependencies
################################################################################
add_dependencies(${PROJECT_NAME}
        common_vc14
        skse64_common
        )

# Link with other targets.
target_link_libraries(${PROJECT_NAME} PUBLIC
        common_vc14
        skse64_common
        )

