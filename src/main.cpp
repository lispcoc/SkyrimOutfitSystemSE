#include <ShlObj.h>

#include "OutfitSystem.h"
//#include "ArmorAddonOverrideService.h"
//#include "PlayerSkinning.h"

std::uint32_t g_pluginSerializationSignature = 'cOft';

void Callback_Messaging_SKSE(SKSEMessagingInterface::Message *message);
void Callback_Serialization_Save(SKSESerializationInterface *intfc);
void Callback_Serialization_Load(SKSESerializationInterface *intfc);

void _assertWrite(bool result, const char *err);
void _assertRead(bool result, const char *err);

void WaitForDebugger(void) {
    while (!IsDebuggerPresent()) {
        Sleep(10);
    }

    Sleep(1000 * 2);
}

namespace {
    void InitializeLog() {
        auto path = logger::log_directory();
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
DllExport bool SKSEPlugin_Query(const SKSEInterface *a_skse, PluginInfo *a_info) {
    logger::info("SkyrimOutfitSystemSE v%s", SKYRIMOUTFITSYSTEMSE_VERSION_VERSTRING);

    a_info->infoVersion = PluginInfo::kInfoVersion;
    a_info->name = "SkyrimOutfitSystemSE";
    a_info->version = 1;

    g_pluginHandle = a_skse->GetPluginHandle();

    if (a_skse->isEditor) {
        logger::error("[FATAL ERROR] Loaded in editor, marking as incompatible!\n");
        return false;
    }

    // NOTE: Version check is disabled since we're trying to use the Address Library
    // if (a_skse->runtimeVersion > MAKE_EXE_VERSION(1, 5, 97) || a_skse->runtimeVersion < MAKE_EXE_VERSION(1, 5, 73))
    // {
    //     _FATALERROR("[FATAL ERROR] Unsupported runtime version %08X!\n", a_skse->runtimeVersion);
    //     return false;
    // }

    g_Messaging = static_cast<SKSEMessagingInterface *>(a_skse->QueryInterface(kInterface_Messaging));
    if (!g_Messaging) {
        _FATALERROR("[FATAL ERROR] Couldn't get messaging interface.");
        return false;
    }
    if (g_Messaging->interfaceVersion < SKSEMessagingInterface::kInterfaceVersion) {
        _FATALERROR("[FATAL ERROR] Messaging interface too old.");
        return false;
    }

    g_Serialization = static_cast<SKSESerializationInterface *>(a_skse->QueryInterface(kInterface_Serialization));
    if (!g_Serialization) {
        _FATALERROR("[FATAL ERROR] Couldn't get serialization interface.");
        return false;
    }
    if (g_Serialization->version < SKSESerializationInterface::kVersion) {
        _FATALERROR("[FATAL ERROR] Serialization interface too old.");
        return false;
    }

    if (!g_branchTrampoline.Create(1024 * 64)) {
        _FATALERROR("couldn't create branch trampoline. this is fatal. skipping remainder of init process.");
        return false;
    }

    if (!g_localTrampoline.Create(1024 * 64, nullptr)) {
        _FATALERROR("couldn't create codegen buffer. this is fatal. skipping remainder of init process.");
        return false;
    }

    return true;
}

void _RegisterAndEchoPapyrus(SKSEPapyrusInterface::RegisterFunctions callback, char *module) {
    bool status = g_Papyrus->Register(callback);
    if (status)
        LOG(info, "Papyrus registration %s for %s.", "succeeded", module);
    else
        LOG(info, "Papyrus registration %s for %s.", "FAILED", module);
} ;

DllExport bool SKSEPlugin_Load(const SKSE::LoadInterface *a_skse) {
    SKSE::Init(a_skse);
    LOG(info, "loading");
    {  // Patches:
        OutfitSystem::ApplyPlayerSkinningHooks();
    }
    {  // Messaging callbacks.
        SKSE::GetMessagingInterface()->RegisterListener();
        g_Messaging->RegisterListener(g_pluginHandle, "SKSE", Callback_Messaging_SKSE);
    }
    {  // Serialization callbacks.
        g_Serialization->SetUniqueID(g_pluginHandle, g_pluginSerializationSignature);
        //g_ISKSESerialization->SetRevertCallback  (g_pluginHandle, Callback_Serialization_Revert);
        g_Serialization->SetSaveCallback(g_pluginHandle, Callback_Serialization_Save);
        g_Serialization->SetLoadCallback(g_pluginHandle, Callback_Serialization_Load);
    }
    {  // Papyrus registrations
        g_Papyrus = (SKSEPapyrusInterface *) a_skse->QueryInterface(kInterface_Papyrus);
        char name[] = "SkyrimOutfitSystemNativeFuncs";
        _RegisterAndEchoPapyrus(OutfitSystem::RegisterPapyrus, name);
    }
    return true;
}
};

void Callback_Messaging_SKSE(SKSEMessagingInterface::Message *message) {
    if (message->type == SKSEMessagingInterface::kMessage_PostLoad) {
    } else if (message->type == SKSEMessagingInterface::kMessage_PostPostLoad) {
    } else if (message->type == SKSEMessagingInterface::kMessage_DataLoaded) {
    } else if (message->type == SKSEMessagingInterface::kMessage_NewGame) {
        ArmorAddonOverrideService::GetInstance().reset();
    } else if (message->type == SKSEMessagingInterface::kMessage_PreLoadGame) {
        ArmorAddonOverrideService::GetInstance()
            .reset(); // AAOS::load resets as well, but this is needed in case the save we're about to load doesn't have any AAOS data.
    }
};
void Callback_Serialization_Save(SKSESerializationInterface *intfc) {
    LOG(info, "Writing savedata...");
    //
    if (intfc->OpenRecord(ArmorAddonOverrideService::signature, ArmorAddonOverrideService::kSaveVersionV4)) {
        try {
            auto &service = ArmorAddonOverrideService::GetInstance();
            const auto &data = service.save(intfc);
            const auto &data_ser = data.SerializeAsString();
            _assertWrite(intfc->WriteRecordData(data_ser.data(), static_cast<std::uint32_t>(data_ser.size())),
                         "Failed to write proto into SKSE record.");
        }
        catch (const ArmorAddonOverrideService::save_error &exception) {
            LOG(info, "Save FAILED for ArmorAddonOverrideService.");
            LOG(info, " - Exception string: %s", exception.what());
        }
    } else
        LOG(info, "Save FAILED for ArmorAddonOverrideService. Record didn't open.");
    //
    LOG(info, "Saving done!");
}
void Callback_Serialization_Load(SKSESerializationInterface *intfc) {
    LOG(info, "Loading savedata...");
    //
    std::uint32_t
        type;    // This IS correct. A std::uint32_t and a four-character ASCII string have the same length (and can be read interchangeably, it seems).
    std::uint32_t version;
    std::uint32_t length;
    bool error = false;
    //
    while (!error && intfc->GetNextRecordInfo(&type, &version, &length)) {
        switch (type) {
        case ArmorAddonOverrideService::signature:
            try {
                auto &service = ArmorAddonOverrideService::GetInstance();
                if (version >= ArmorAddonOverrideService::kSaveVersionV4) {
                    using namespace Serialization;
                    // Read data from protobuf.
                    std::vector<char> buf;
                    buf.insert(buf.begin(), length, 0);
                    _assertRead(intfc->ReadRecordData(buf.data(), length) == length, "Failed to load protobuf.");

                    // Parse data in protobuf.
                    proto::OutfitSystem data;
                    _assertRead(data.ParseFromArray(buf.data(), static_cast<int>(buf.size())),
                                "Failed to parse protobuf.");

                    // Load data from protobuf struct.
                    service.load(intfc, data);
                } else {
                    service.load_legacy(intfc, version);
                }
            }
            catch (const ArmorAddonOverrideService::load_error &exception) {
                LOG(info, "Load FAILED for ArmorAddonOverrideService.");
                LOG(info, " - Exception string: %s", exception.what());
            }
            break;
        default:
            LOG(info, "Loading: Unhandled type %c%c%c%c",
                     (char) (type >> 0x18),
                     (char) (type >> 0x10),
                     (char) (type >> 0x8),
                     (char) type);
            error = true;
            break;
        }
    }
    //
    LOG(info, "Loading done!");
}