################################################################################
################################################################################
## common_vc14
################################################################################
################################################################################
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

list(TRANSFORM ALL_FILES PREPEND "./skse64/src/common/")

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
set(no_group_source_files
        "BranchTrampoline.cpp"
        "BranchTrampoline.h"
        "Relocation.cpp"
        "Relocation.h"
        "SafeWrite.cpp"
        "SafeWrite.h"
        "skse_version.h"
        "Utilities.cpp"
        "Utilities.h"
        )
source_group("" FILES ${no_group_source_files})

set(ALL_FILES
        ${no_group_source_files}
        )

list(TRANSFORM ALL_FILES PREPEND "./skse64/src/skse64/skse64_common/")

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
set(no_group_source_files
        "skse64.cpp"
        )
source_group("" FILES ${no_group_source_files})

set(api
        "GameAPI.cpp"
        "GameAPI.h"
        "GameBSExtraData.cpp"
        "GameBSExtraData.h"
        "GameCamera.cpp"
        "GameCamera.h"
        "GameData.cpp"
        "GameData.h"
        "GameEvents.cpp"
        "GameEvents.h"
        "GameExtraData.cpp"
        "GameExtraData.h"
        "GameFormComponents.cpp"
        "GameFormComponents.h"
        "GameForms.cpp"
        "GameForms.h"
        "GameHandlers.cpp"
        "GameHandlers.h"
        "GameInput.cpp"
        "GameInput.h"
        "GameMenus.cpp"
        "GameMenus.h"
        "GameObjects.cpp"
        "GameObjects.h"
        "GameReferences.cpp"
        "GameReferences.h"
        "GameResources.cpp"
        "GameResources.h"
        "GameRTTI.cpp"
        "GameRTTI.h"
        "GameRTTI.inl"
        "GameSettings.cpp"
        "GameSettings.h"
        "GameStreams.cpp"
        "GameStreams.h"
        "GameThreads.cpp"
        "gamethreads.h"
        "GameTypes.cpp"
        "GameTypes.h"
        "GameUtilities.cpp"
        "GameUtilities.h"
        "ObScript.cpp"
        "ObScript.h"
        )
source_group("api" FILES ${api})

set(hooks
        "Hooks_Camera.cpp"
        "Hooks_Camera.h"
        "Hooks_Data.cpp"
        "Hooks_Data.h"
        "Hooks_Debug.cpp"
        "Hooks_Debug.h"
        "Hooks_Diagnostics.cpp"
        "Hooks_Diagnostics.h"
        "Hooks_DirectInput8Create.cpp"
        "Hooks_DirectInput8Create.h"
        "Hooks_Event.cpp"
        "Hooks_Event.h"
        "Hooks_Gameplay.cpp"
        "Hooks_Gameplay.h"
        "Hooks_Handlers.cpp"
        "Hooks_Handlers.h"
        "Hooks_NetImmerse.cpp"
        "Hooks_NetImmerse.h"
        "Hooks_ObScript.cpp"
        "Hooks_ObScript.h"
        "Hooks_Papyrus.cpp"
        "Hooks_Papyrus.h"
        "Hooks_SaveLoad.cpp"
        "Hooks_SaveLoad.h"
        "Hooks_Scaleform.cpp"
        "Hooks_Scaleform.h"
        "Hooks_Threads.cpp"
        "Hooks_Threads.h"
        "Hooks_UI.cpp"
        "Hooks_UI.h"
        )
source_group("hooks" FILES ${hooks})

set(internal
        "Colors.cpp"
        "Colors.h"
        "CustomMenu.cpp"
        "CustomMenu.h"
        "GlobalLocks.cpp"
        "GlobalLocks.h"
        "HashUtil.cpp"
        "HashUtil.h"
        "InputMap.cpp"
        "InputMap.h"
        "InternalSerialization.cpp"
        "InternalSerialization.h"
        "InternalTasks.cpp"
        "InternalTasks.h"
        "PluginAPI.h"
        "PluginManager.cpp"
        "PluginManager.h"
        "Serialization.cpp"
        "Serialization.h"
        "Translation.cpp"
        "Translation.h"
        )
source_group("internal" FILES ${internal})

set(netimmerse
        "BSModelDB.cpp"
        "BSModelDB.h"
        "NiAdditionalGeometryData.cpp"
        "NiAdditionalGeometryData.h"
        "NiControllers.cpp"
        "NiControllers.h"
        "NiExtraData.cpp"
        "NiExtraData.h"
        "NiGeometry.cpp"
        "NiGeometry.h"
        "NiLight.cpp"
        "NiLight.h"
        "NiMaterial.cpp"
        "NiMaterial.h"
        "NiNodes.cpp"
        "NiNodes.h"
        "NiObjects.cpp"
        "NiObjects.h"
        "NiProperties.cpp"
        "NiProperties.h"
        "NiRenderer.cpp"
        "NiRenderer.h"
        "NiRTTI.cpp"
        "NiRTTI.h"
        "NiSerialization.cpp"
        "NiSerialization.h"
        "NiTextures.cpp"
        "NiTextures.h"
        "NiTypes.cpp"
        "NiTypes.h"
        )
source_group("netimmerse" FILES ${netimmerse})

set(papyrus__functions
        "PapyrusActiveMagicEffect.cpp"
        "PapyrusActiveMagicEffect.h"
        "PapyrusActor.cpp"
        "PapyrusActor.h"
        "PapyrusActorBase.cpp"
        "PapyrusActorBase.h"
        "PapyrusActorValueInfo.cpp"
        "PapyrusActorValueInfo.h"
        "PapyrusAlias.cpp"
        "PapyrusAlias.h"
        "PapyrusAmmo.cpp"
        "PapyrusAmmo.h"
        "PapyrusArmor.cpp"
        "PapyrusArmor.h"
        "PapyrusArmorAddon.cpp"
        "PapyrusArmorAddon.h"
        "PapyrusArt.cpp"
        "PapyrusArt.h"
        "PapyrusBook.cpp"
        "PapyrusBook.h"
        "PapyrusCamera.cpp"
        "PapyrusCamera.h"
        "PapyrusCell.cpp"
        "PapyrusCell.h"
        "PapyrusClass.cpp"
        "PapyrusClass.h"
        "PapyrusColorForm.cpp"
        "PapyrusColorForm.h"
        "PapyrusCombatStyle.cpp"
        "PapyrusCombatStyle.h"
        "PapyrusConstructibleObject.cpp"
        "PapyrusConstructibleObject.h"
        "PapyrusDefaultObjectManager.cpp"
        "PapyrusDefaultObjectManager.h"
        "PapyrusEnchantment.cpp"
        "PapyrusEnchantment.h"
        "PapyrusEquipSlot.cpp"
        "PapyrusEquipSlot.h"
        "PapyrusFaction.cpp"
        "PapyrusFaction.h"
        "PapyrusFlora.cpp"
        "PapyrusFlora.h"
        "PapyrusForm.cpp"
        "PapyrusForm.h"
        "PapyrusFormList.cpp"
        "PapyrusFormList.h"
        "PapyrusGame.cpp"
        "PapyrusGame.h"
        "PapyrusGameData.cpp"
        "PapyrusGameData.h"
        "PapyrusHeadPart.cpp"
        "PapyrusHeadPart.h"
        "PapyrusIngredient.cpp"
        "PapyrusIngredient.h"
        "PapyrusInput.cpp"
        "PapyrusInput.h"
        "PapyrusKeyword.cpp"
        "PapyrusKeyword.h"
        "PapyrusLeveledActor.cpp"
        "PapyrusLeveledActor.h"
        "PapyrusLeveledItem.cpp"
        "PapyrusLeveledItem.h"
        "PapyrusLeveledSpell.cpp"
        "PapyrusLeveledSpell.h"
        "PapyrusLocation.cpp"
        "PapyrusLocation.h"
        "PapyrusMagicEffect.cpp"
        "PapyrusMagicEffect.h"
        "PapyrusMath.cpp"
        "PapyrusMath.h"
        "PapyrusMisc.cpp"
        "PapyrusMisc.h"
        "PapyrusModEvent.cpp"
        "PapyrusModEvent.h"
        "PapyrusNetImmerse.cpp"
        "PapyrusNetImmerse.h"
        "PapyrusObjectReference.cpp"
        "PapyrusObjectReference.h"
        "PapyrusPerk.cpp"
        "PapyrusPerk.h"
        "PapyrusPotion.cpp"
        "PapyrusPotion.h"
        "PapyrusQuest.cpp"
        "PapyrusQuest.h"
        "PapyrusRace.cpp"
        "PapyrusRace.h"
        "PapyrusReferenceAlias.cpp"
        "PapyrusReferenceAlias.h"
        "PapyrusScroll.cpp"
        "PapyrusScroll.h"
        "PapyrusShout.cpp"
        "PapyrusShout.h"
        "PapyrusSKSE.cpp"
        "PapyrusSKSE.h"
        "PapyrusSound.cpp"
        "PapyrusSound.h"
        "PapyrusSoundDescriptor.cpp"
        "PapyrusSoundDescriptor.h"
        "PapyrusSpawnerTask.cpp"
        "PapyrusSpawnerTask.h"
        "PapyrusSpell.cpp"
        "PapyrusSpell.h"
        "PapyrusStringUtil.cpp"
        "PapyrusStringUtil.h"
        "PapyrusTextureSet.cpp"
        "PapyrusTextureSet.h"
        "PapyrusTree.cpp"
        "PapyrusTree.h"
        "PapyrusUI.cpp"
        "PapyrusUI.h"
        "PapyrusUICallback.cpp"
        "PapyrusUICallback.h"
        "PapyrusUtility.cpp"
        "PapyrusUtility.h"
        "PapyrusWeapon.cpp"
        "PapyrusWeapon.h"
        "PapyrusWeather.cpp"
        "PapyrusWeather.h"
        "PapyrusWornObject.cpp"
        "PapyrusWornObject.h"
        )
source_group("papyrus\\functions" FILES ${papyrus__functions})

set(papyrus__vm
        "PapyrusArgs.cpp"
        "PapyrusArgs.h"
        "PapyrusDelayFunctors.cpp"
        "PapyrusDelayFunctors.h"
        "PapyrusEventFunctor.h"
        "PapyrusEvents.cpp"
        "PapyrusEvents.h"
        "PapyrusInterfaces.cpp"
        "PapyrusInterfaces.h"
        "PapyrusNativeFunctionDef.inl"
        "PapyrusNativeFunctionDef_Base.inl"
        "PapyrusNativeFunctions.cpp"
        "PapyrusNativeFunctions.h"
        "PapyrusObjects.cpp"
        "PapyrusObjects.h"
        "PapyrusValue.cpp"
        "PapyrusValue.h"
        "PapyrusVM.cpp"
        "PapyrusVM.h"
        )
source_group("papyrus\\vm" FILES ${papyrus__vm})

set(scaleform
        "ScaleformAPI.cpp"
        "ScaleformAPI.h"
        "ScaleformCallbacks.cpp"
        "ScaleformCallbacks.h"
        "ScaleformExtendedData.cpp"
        "ScaleformExtendedData.h"
        "ScaleformLoader.cpp"
        "ScaleformLoader.h"
        "ScaleformMovie.cpp"
        "ScaleformMovie.h"
        "ScaleformState.cpp"
        "ScaleformState.h"
        "ScaleformTypes.cpp"
        "ScaleformTypes.h"
        "ScaleformValue.cpp"
        "ScaleformValue.h"
        "ScaleformVM.cpp"
        "ScaleformVM.h"
        )
source_group("scaleform" FILES ${scaleform})

set(ALL_FILES
        ${no_group_source_files}
        ${api}
        ${hooks}
        ${internal}
        ${netimmerse}
        ${papyrus__functions}
        ${papyrus__vm}
        ${scaleform}
        )

list(TRANSFORM ALL_FILES PREPEND "./skse64/src/skse64/skse64/")

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
        "RUNTIME_VERSION=0x01050610"
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

