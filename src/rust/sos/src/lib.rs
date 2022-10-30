mod helpers;
mod interface;
mod logging;
pub mod outfit;
mod persistence;
pub mod strings;

use crate::logging::SimpleLogger;
use crate::outfit::OutfitService;
use commonlibsse::*;
use log::*;

#[no_mangle]
pub extern "C" fn plugin_main(skse: *const SKSE_LoadInterface) -> bool {
    unsafe {
        REL_Module::reset();
        InitializeLog();
        SimpleLogger::setup();

        info!("Load SkyrimOutfitSystem");
        info!("Game Type: {}", REL_Relocate("SE", "AE"));
        info!("Game Runtime Version: {}", "unavailable");

        SKSE_Init(skse);

        if (*SKSE_GetMessagingInterface()).Version() < SKSE_MessagingInterface_kVersion as u32 {
            error!("Messaging interface too old");
            return false;
        };

        if (*SKSE_GetSerializationInterface()).Version()
            < SKSE_SerializationInterface_kVersion as u32
        {
            error!("Serialization interface too old");
            return false;
        };

        InitializeTrampolines();

        info!("Patching player skinning");
        REL_Relocate(
            ApplyPlayerSkinningHooksSE as unsafe extern "C" fn(),
            ApplyPlayerSkinningHooksAE as unsafe extern "C" fn(),
        )();

        // Messaging Callback
        info!("Registering messaging callback");
        (*SKSE_GetMessagingInterface()).RegisterListener(Some(messaging_callback));

        // Serialization Callbacks
        info!("Registering serialization callback");
        (*SKSE_GetSerializationInterface()).SetUniqueID(persistence::UNIQUE_SIGNATURE_INT);
        (*SKSE_GetSerializationInterface())
            .SetSaveCallback(Some(persistence::serialization_save_callback));
        (*SKSE_GetSerializationInterface())
            .SetLoadCallback(Some(persistence::serialization_load_callback));

        // Papyrus Registrations
        info!("Registering papyrus");
        SetupPapyrus();

        true
    }
}

#[no_mangle]
#[allow(non_upper_case_globals)]
pub extern "C" fn messaging_callback(message: *mut SKSE_MessagingInterface_Message) {
    if message.is_null() {
        return;
    }
    let message_type = unsafe { (*message).type_ };
    match message_type {
        SKSE_MessagingInterface_kPostLoad => {}
        SKSE_MessagingInterface_kPostPostLoad => {}
        SKSE_MessagingInterface_kDataLoaded => {}
        SKSE_MessagingInterface_kNewGame | SKSE_MessagingInterface_kPreLoadGame => {
            let service = unsafe { &mut *outfit_service_get_singleton_ptr() };
            service.replace_with_new();
            service.check_consistency();
        }
        message_type => {
            warn!("Got unknown message type {}", message_type);
        }
    }
}

static mut OUTFIT_SERVICE_SINGLETON: Option<*mut OutfitService> = None;

fn outfit_service_get_singleton_ptr() -> *mut OutfitService {
    unsafe {
        if OUTFIT_SERVICE_SINGLETON.is_none() {
            let leaked = Box::into_raw(Box::new(OutfitService::new()));
            OUTFIT_SERVICE_SINGLETON = Some(leaked);
        };
        OUTFIT_SERVICE_SINGLETON.clone().unwrap()
    }
}

extern "C" {
    fn InitializeLog() -> bool;
    fn InitializeTrampolines();
    fn ApplyPlayerSkinningHooksSE();
    fn ApplyPlayerSkinningHooksAE();
    fn SetupPapyrus();
}
