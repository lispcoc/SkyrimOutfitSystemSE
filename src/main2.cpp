#include "OutfitSystem.h"

#include <ShlObj.h>

void WaitForDebugger(void) {
    while (!IsDebuggerPresent()) {
        Sleep(10);
    }

    Sleep(1000 * 2);
}

namespace {
    void InitializeLog() {
        auto path = SKSE::log::log_directory();
        if (!path) {
            util::report_and_fail("Failed to find standard logging directory"sv);
        }

        *path /= fmt::format("{}.log"sv, Plugin::NAME);
        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

#ifndef NDEBUG
        const auto level = spdlog::level::trace;
#else
        const auto level = spdlog::level::info;
#endif

        auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
        log->set_level(level);
        log->flush_on(level);

        spdlog::set_default_logger(std::move(log));
        spdlog::set_pattern("%g(%#): [%^%l%$] %v"s);
    }
}

extern "C" {
// Plugin Query for AE
DllExport constinit auto SKSEPlugin_Version = []() {
    SKSE::PluginVersionData v;

    v.PluginVersion(Plugin::VERSION);
    v.PluginName(Plugin::NAME);

    v.UsesAddressLibrary(true);
    v.CompatibleVersions({SKSE::RUNTIME_LATEST});

    return v;
}();

// Plugin Query for SE
DllExport bool SKSEPlugin_Query(const SKSE::QueryInterface *a_skse, SKSE::PluginInfo *a_info) {
    LOG(info, "Query: {} v{}", Plugin::NAME, Plugin::VERSION.string());
}

// Entry point
DllExport bool SKSEPlugin_Load(const SKSE::LoadInterface *a_skse) {
    InitializeLog();
    LOG(info, "{} v{}", Plugin::NAME, Plugin::VERSION.string());

    SKSE::Init(a_skse);

    return true;
}
}