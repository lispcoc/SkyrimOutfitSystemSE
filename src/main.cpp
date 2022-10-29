#include "OutfitSystem.h"

#include "ArmorAddonOverrideService.h"
#include "Utility.h"
#include "hooking/Hooks.hpp"

void WaitForDebugger(void) {
    while (!IsDebuggerPresent()) {
        Sleep(10);
    }

    Sleep(1000 * 2);
}

int ReportHook(int reportType, char* message, int* returnValue) {
    // Got an error
    util::report_and_fail(message);
}

extern "C" {
void InitializeLog() {
    auto path = SKSE::log::log_directory();
    if (!path) {
        util::report_and_fail("Failed to find standard logging directory"sv);
    }

    *path /= fmt::format("{}.log"sv, Plugin::NAME);
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

    // Activate logging of everything until we load the log setting.
    auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
    log->set_level(spdlog::level::trace);
    log->flush_on(spdlog::level::trace);

    spdlog::set_default_logger(std::move(log));
    spdlog::set_pattern("%g(%#): [%^%l%$] %v"s);

    // Load the actual log setting we should use.
    auto level = spdlog::level::info;
    bool deepLogEnabled = Settings::Instance()->GetBoolean("Debug", "ExtraLogging", false);
    if (deepLogEnabled) {
        LOG(info, "Extra logging enabled.");
        level = spdlog::level::trace;
    } else {
        LOG(info, "Extra logging disabled.");
    }

    // Set the actual log setting from hereon.
    spdlog::default_logger()->set_level(level);
    spdlog::default_logger()->flush_on(level);
}
int RustGetLogLevel() {
    return spdlog::default_logger()->level();
}
void RustLog(const char *filename_in, int line_in, const char *funcname_in, int level, const char* message) {
    spdlog::default_logger()->log(spdlog::source_loc(filename_in, line_in, funcname_in), spdlog::level::info, message);
}
void InitializeTrampolines() {
    SKSE::AllocTrampoline(128, true);
    Hooking::g_branchTrampoline = &SKSE::GetTrampoline();

    Hooking::g_localTrampoline = new SKSE::Trampoline("local");
    Hooking::g_localTrampoline->create(1024 * 64);

}
void SetupPapyrus() {
    SKSE::GetPapyrusInterface()->Register(OutfitSystem::RegisterPapyrus);
}
}// namespace


extern "C" {
// Plugin Query for AE
DllExport constinit auto SKSEPlugin_Version = []() {
    SKSE::PluginVersionData v;

    v.PluginVersion(Plugin::VERSION);
    v.PluginName(Plugin::NAME);

    v.UsesAddressLibrary(true);
    v.CompatibleVersions({SKSE::RUNTIME_SSE_LATEST_AE});
    v.UsesNoStructs(true);

    return v;
}();

// Plugin Query for SE
DllExport bool SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info) {
    a_info->infoVersion = SKSE::PluginInfo::kVersion;
    a_info->name = "SkyrimOutfitSystemSE";
    a_info->version = 1;

    if (a_skse->IsEditor()) {
        LOG(critical, "[FATAL ERROR] Loaded in editor, marking as incompatible!\n");
        return false;
    }

    return true;
}
bool plugin_main(const SKSE::LoadInterface* a_skse);
DllExport bool SKSEPlugin_Load(const SKSE::LoadInterface* a_skse) {
    return plugin_main(a_skse);
}
}
