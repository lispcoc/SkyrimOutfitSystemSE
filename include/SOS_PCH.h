//
// Created by m on 10/30/2020.
//

#ifndef SKYRIMOUTFITSYSTEMSE_SOS_PCH_H
#define SKYRIMOUTFITSYSTEMSE_SOS_PCH_H

#include "version.h"

#pragma warning(push)
#include "SKSE/Impl/PCH.h"
#include <RE/Skyrim.h>
#include <REL/Relocation.h>
#include <SKSE/SKSE.h>

#ifdef NDEBUG
#	include <spdlog/sinks/basic_file_sink.h>
#else
#	include <spdlog/sinks/basic_file_sink.h>
#endif
#pragma warning(pop)

using namespace std::literals;

#define LOG(a_type, ...) spdlog::log(spdlog::source_loc(__FILE__, __LINE__, __FUNCTION__), spdlog::level::a_type, __VA_ARGS__)

namespace util {
    using SKSE::stl::report_and_fail;
}

#define DllExport __declspec( dllexport )

namespace Plugin {
    using namespace std::literals;

#if SKYRIM_VERSION_IS_SOME_AE
    inline constexpr REL::Version VERSION{SKYRIMOUTFITSYSTEMSE_VERSION_MAJOR, SKYRIMOUTFITSYSTEMSE_VERSION_MINOR, SKYRIMOUTFITSYSTEMSE_VERSION_PATCH};
#elif SKYRIM_VERSION_IS_PRE_AE
    inline constexpr REL::Version VERSION{SKYRIMOUTFITSYSTEMSE_VERSION_MAJOR, SKYRIMOUTFITSYSTEMSE_VERSION_MINOR, SKYRIMOUTFITSYSTEMSE_VERSION_PATCH, 0};
#endif
    inline constexpr auto NAME = "SkyrimOutfitSystemSE"sv;
}

namespace RE {
    using RawActorHandle = RE::ActorHandle::native_handle_type;
}

#endif //SKYRIMOUTFITSYSTEMSE_SOS_PCH_H
