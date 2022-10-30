use crate::outfit::{policy::*, *};
use crate::OUTFIT_SERVICE_SINGLETON;
use crate::strings::*;
use crate::settings::SETTINGS;
use std::sync::MutexGuard;

#[cxx::bridge]
pub mod ffi {
    pub struct WeatherFlags {
        pub rainy: bool,
        pub snowy: bool,
    }

    #[derive(Ord, PartialOrd, Eq, PartialEq, Copy, Clone)]
    #[repr(u32)]
    pub enum LocationType {
        World = 0,
        Town = 1,
        Dungeon = 2,
        City = 9,

        WorldSnowy = 3,
        TownSnowy = 4,
        DungeonSnowy = 5,
        CitySnowy = 10,

        WorldRainy = 6,
        TownRainy = 7,
        DungeonRainy = 8,
        CityRainy = 11,
    }

    #[derive(Ord, PartialOrd, Eq, PartialEq)]
    #[repr(u8)]
    pub enum Policy {
        XXXX,
        XXXE,
        XXXO,
        XXOX,
        XXOE,
        XXOO,
        XEXX,
        XEXE,
        XEXO,
        XEOX,
        XEOE,
        XEOO,
    }

    pub struct OptionalLocationType {
        pub has_value: bool,
        pub value: LocationType,
    }

    pub struct OptionalPolicy {
        pub has_value: bool,
        pub value: Policy,
    }

    pub struct TESObjectARMOPtr {
        pub ptr: *mut TESObjectARMO,
    }

    #[derive(Clone)]
    pub struct MetadataC {
        pub value: Policy,
        pub code_buf: *const c_char,
        pub code_len: usize,
        pub sort_order: i8,
        pub advanced: bool,
    }

    pub struct OptionalMetadata {
        pub has_value: bool,
        pub value: MetadataC,
    }

    #[namespace = "RE"]
    extern "C++" {
        include!("sos/include/customize.h");
        #[namespace = "SKSE"]
        type SerializationInterface = commonlibsse::SKSE_SerializationInterface;
        type TESObjectARMO = commonlibsse::RE_TESObjectARMO;
    }
    unsafe extern "C++" {
        include!("sos/include/customize.h");
        // fn GetRuntimePath() -> UniquePtr<CxxString>;
        // fn GetRuntimeName() -> UniquePtr<CxxString>;
        fn GetRuntimeDirectory() -> UniquePtr<CxxString>;
    }
    extern "Rust" {
        type OutfitSystemMutex;
        fn outfit_service_get_singleton_ptr() -> Box<OutfitSystemMutex>;
        fn inner(self: &mut OutfitSystemMutex) -> &mut OutfitService;
        type OutfitService;
        unsafe fn replace_with_json_data(
            self: &mut OutfitService,
            data: &str,
            intfc: *const SerializationInterface,
        ) -> bool;
        fn max_outfit_name_len(self: &OutfitService) -> i32;
        fn get_outfit_ptr(self: &mut OutfitService, name: &str) -> *mut Outfit;
        fn get_or_create_mut_outfit_ptr(self: &mut OutfitService, name: &str) -> *mut Outfit;
        fn add_outfit(self: &mut OutfitService, name: &str);
        fn current_outfit_ptr(self: &mut OutfitService, target: u32) -> *mut Outfit;
        fn has_outfit(self: &OutfitService, name: &str) -> bool;
        fn delete_outfit(self: &mut OutfitService, name: &str);
        fn set_favorite(self: &mut OutfitService, name: &str, favorite: bool);
        unsafe fn modify_outfit(
            self: &mut OutfitService,
            name: &str,
            add: &[*mut TESObjectARMO],
            remove: &[*mut TESObjectARMO],
            create_if_needed: bool,
        );
        fn rename_outfit(self: &mut OutfitService, old_name: &str, new_name: &str) -> u32;
        fn set_outfit_c(self: &mut OutfitService, name: &str, target: u32);
        fn add_actor(self: &mut OutfitService, target: u32);
        fn remove_actor(self: &mut OutfitService, target: u32);
        fn list_actors(self: &OutfitService) -> Vec<u32>;
        fn set_location_based_switching_enabled(self: &mut OutfitService, setting: bool);
        fn set_outfit_using_location(self: &mut OutfitService, location: LocationType, target: u32);
        fn set_location_outfit(
            self: &mut OutfitService,
            location: LocationType,
            target: u32,
            name: &str,
        );
        fn get_location_based_switching_enabled(self: &OutfitService) -> bool;
        fn unset_location_outfit(self: &mut OutfitService, location: LocationType, target: u32);
        fn get_location_outfit_name_c(
            self: &OutfitService,
            location: LocationType,
            target: u32,
        ) -> String;
        fn check_location_type_c(
            self: &OutfitService,
            keywords: Vec<String>,
            weather_flags: WeatherFlags,
            target: u32,
        ) -> OptionalLocationType;
        fn should_override(self: &OutfitService, target: u32) -> bool;
        fn get_outfit_names(self: &OutfitService, favorites_only: bool) -> Vec<String>;
        fn set_enabled(self: &mut OutfitService, option: bool);
        fn enabled_c(self: &OutfitService) -> bool;
        fn save_json_c(self: &mut OutfitService) -> String;

        type Outfit;
        unsafe fn conflicts_with(self: &Outfit, armor: *mut TESObjectARMO) -> bool;
        unsafe fn compute_display_set_c(
            self: &Outfit,
            equipped: &[*mut TESObjectARMO],
        ) -> Vec<TESObjectARMOPtr>;
        fn set_slot_policy_c(self: &mut Outfit, slot: u32, policy: OptionalPolicy);
        fn set_blanket_slot_policy(self: &mut Outfit, policy: Policy);
        fn reset_to_default_slot_policy(self: &mut Outfit);
        fn armors_c(self: &Outfit) -> Vec<TESObjectARMOPtr>;
        fn favorite_c(self: &Outfit) -> bool;
        fn name_c(self: &Outfit) -> String;
        unsafe fn insert_armor(self: &mut Outfit, armor: *mut TESObjectARMO);
        unsafe fn erase_armor(self: &mut Outfit, armor: *mut TESObjectARMO);
        fn erase_all_armors(self: &mut Outfit);
        fn policy_names_for_outfit(self: &Outfit) -> Vec<String>;

        fn is_form_id_permitted(form: u32) -> bool;

        // Policy methods
        fn policy_with_code_c(code: &str) -> OptionalPolicy;
        fn list_available_policies_c(allow_advanced: bool) -> Vec<MetadataC>;
        fn translation_key_c(policy: &Policy) -> String;
        fn policy_metadata_c(policy: &Policy) -> OptionalMetadata;

        // String Utilities
        fn nat_ord_case_insensitive_c(a: &str, b: &str) -> i8;

        // Settings
        fn settings_extra_logging_enabled() -> bool;
    }
}

fn settings_extra_logging_enabled() -> bool {
    SETTINGS.extra_logging_enabled()
}

struct OutfitSystemMutex {
    inner: MutexGuard<'static, OutfitService>
}

impl OutfitSystemMutex {
    pub fn inner(&mut self) -> &mut OutfitService {
        &mut *self.inner
    }
}

fn outfit_service_get_singleton_ptr() -> Box<OutfitSystemMutex> {
    Box::new(OutfitSystemMutex { inner: OUTFIT_SERVICE_SINGLETON.lock().expect("OutfitService mutex poisoned") })
}
