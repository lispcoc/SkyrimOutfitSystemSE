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


target_precompile_headers(${PROJECT_NAME} PRIVATE
        ./skse64/src/common/IPrefix.h
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
set(ALL_FILES ${HEADER_FILES} ${SRC_FILES})

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
        ./skse64/src/common/IPrefix.h
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
        ./skse64/src/common/IPrefix.h
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

