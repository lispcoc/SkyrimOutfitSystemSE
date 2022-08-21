//
// Created by m on 10/30/2020.
//

#ifndef SKYRIMOUTFITSYSTEMSE_SOS_PCH_H
#define SKYRIMOUTFITSYSTEMSE_SOS_PCH_H

#pragma warning(push)
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

#ifdef NDEBUG
#	include <spdlog/sinks/basic_file_sink.h>
#else
#	include <spdlog/sinks/basic_file_sink.h>
#endif
#pragma warning(pop)

using namespace std::literals;

namespace logger = SKSE::log;

namespace util
{
using SKSE::stl::report_and_fail;
}

#define DllExport   __declspec( dllexport )

namespace Plugin {
    using namespace std::literals;

    inline constexpr REL::Version VERSION{ 0, 3, 0 };

    inline constexpr auto NAME = "SkyrimOutfitSystemSE"sv;
}

#endif //SKYRIMOUTFITSYSTEMSE_SOS_PCH_H
