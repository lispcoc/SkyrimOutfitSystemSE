//
// Created by m on 10/30/2020.
//

#ifndef SKYRIMOUTFITSYSTEMSE_SOS_PCH_H
#define SKYRIMOUTFITSYSTEMSE_SOS_PCH_H

#include "version.h"

#pragma warning(push)
#if SKYRIM_VERSION_IS_AE
#include "SKSE/Impl/PCH.h"
#include <RE/Skyrim.h>
#include <REL/Relocation.h>
#include <SKSE/SKSE.h>
#include <RE/A/Actor.h>
#include <RE/M/Misc.h>
#include "RE/T/TESObjectARMO.h"
#include "RE/P/PlayerCharacter.h"
#include "RE/A/ActorEquipManager.h"
#include "RE/I/InventoryChanges.h"
#include "RE/I/InventoryEntryData.h"
#include "RE/P/PlayerCharacter.h"
#include "RE/T/TESObjectARMO.h"
#include "RE/T/TESObjectREFR.h"
#include "RE/I/IVirtualMachine.h"
#include "RE/T/TESForm.h"
#include "RE/I/IVirtualMachine.h"
#elif SKYRIM_VERSION_IS_PRE_AE
#include "SKSE/Impl/PCH.h"
#include <RE/Skyrim.h>
#include <REL/Relocation.h>
#include <SKSE/SKSE.h>
#include <RE/A/Actor.h>
#include <RE/M/Misc.h>
#include "RE/T/TESObjectARMO.h"
#include "RE/P/PlayerCharacter.h"
#include "RE/A/ActorEquipManager.h"
#include "RE/I/InventoryChanges.h"
#include "RE/I/InventoryEntryData.h"
#include "RE/P/PlayerCharacter.h"
#include "RE/T/TESObjectARMO.h"
#include "RE/T/TESObjectREFR.h"
#include "RE/I/IVirtualMachine.h"
#include "RE/T/TESForm.h"
#include "RE/I/IVirtualMachine.h"
#endif

#include "fmt/compile.h"

#ifdef NDEBUG
#	include <spdlog/sinks/basic_file_sink.h>
#else
#	include <spdlog/sinks/basic_file_sink.h>
#endif
#pragma warning(pop)

using namespace std::literals;

#define LOG(a_type, ...) spdlog::log(spdlog::source_loc(__FILE__, __LINE__, __FUNCTION__), spdlog::level::a_type, fmt::format(__VA_ARGS__))

namespace util {
    using SKSE::stl::report_and_fail;
}

#define DllExport __declspec( dllexport )

namespace Plugin {
    using namespace std::literals;

#if SKYRIM_VERSION_IS_AE
    inline constexpr REL::Version VERSION{SKYRIMOUTFITSYSTEMSE_VERSION_MAJOR, SKYRIMOUTFITSYSTEMSE_VERSION_MINOR, SKYRIMOUTFITSYSTEMSE_VERSION_PATCH};
#elif SKYRIM_VERSION_IS_PRE_AE
    inline constexpr REL::Version VERSION{SKYRIMOUTFITSYSTEMSE_VERSION_MAJOR, SKYRIMOUTFITSYSTEMSE_VERSION_MINOR, SKYRIMOUTFITSYSTEMSE_VERSION_PATCH, 0};
#endif
    inline constexpr auto NAME = "SkyrimOutfitSystemSE"sv;
}

#endif //SKYRIMOUTFITSYSTEMSE_SOS_PCH_H
