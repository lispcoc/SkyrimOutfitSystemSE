set(PROJECT_NAME common_vc14)

################################################################################
# Source groups
################################################################################
set(no_group_source_files
    "IPrefix.cpp"
    "IPrefix.h"
)
source_group("" FILES ${no_group_source_files})

set(datatypes
    "ISingleton.cpp"
    "ISingleton.h"
    "ITypes.cpp"
    "ITypes.h"
)
source_group("datatypes" FILES ${datatypes})

set(debug
    "IDebugLog.cpp"
    "IDebugLog.h"
    "IErrors.cpp"
    "IErrors.h"
)
source_group("debug" FILES ${debug})

set(streams
    "IBufferStream.cpp"
    "IBufferStream.h"
    "IDataStream.cpp"
    "IDataStream.h"
    "IFileStream.cpp"
    "IFileStream.h"
    "ISegmentStream.cpp"
    "ISegmentStream.h"
)
source_group("streams" FILES ${streams})

set(threads
    "IEvent.cpp"
    "IEvent.h"
    "IInterlockedLong.cpp"
    "IInterlockedLong.h"
    "IMutex.cpp"
    "IMutex.h"
)
source_group("threads" FILES ${threads})

set(utilities
    "IDirectoryIterator.cpp"
    "IDirectoryIterator.h"
    "ITimer.cpp"
    "ITimer.h"
)
source_group("utilities" FILES ${utilities})

set(ALL_FILES
    ${no_group_source_files}
    ${datatypes}
    ${debug}
    ${streams}
    ${threads}
    ${utilities}
)

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
    "${CMAKE_CURRENT_SOURCE_DIR}/..;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../skse64"
)


target_precompile_headers(${PROJECT_NAME} PRIVATE
    IPrefix.h
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

