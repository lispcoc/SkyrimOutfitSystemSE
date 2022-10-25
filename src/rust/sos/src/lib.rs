#![allow(dead_code)]

mod outfit;

use std::string::String;
use uncased::{Uncased};
#[allow(unused_imports)]
use protos::outfit as ProtoOutfit;
use crate::outfit::{OutfitService, Outfit};

type UncasedString = Uncased<'static>;

#[cxx::bridge]
mod ffi {
    #[cxx_name = "RustWeatherFlags"]
    pub struct WeatherFlags {
        pub rainy: bool,
        pub snowy: bool,
    }

    #[cxx_name = "RustLocationType"]
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
        CityRainy = 11
    }

    #[cxx_name = "RustPolicy"]
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
        pub ptr: *mut TESObjectARMO
    }

    #[namespace = "RE"]
    extern "C++" {
        include!("sos/include/customize.h");
        #[namespace = "SKSE"]
        type SerializationInterface = commonlibsse::SerializationInterface;
        type TESObjectARMO = commonlibsse::TESObjectARMO;
        type BIPED_OBJECT = commonlibsse::BIPED_OBJECT;
    }
    extern "Rust" {
        type OutfitService;
        fn outfit_service_create() -> Box<OutfitService>;
        fn replace_with_new(self: &mut OutfitService);
        unsafe fn replace_with_proto_data_ptr(self: &mut OutfitService, data: &[u8], intfc: *const SerializationInterface) -> bool;
        unsafe fn replace_with_json_data(self: &mut OutfitService, data: &str, intfc: *const SerializationInterface) -> bool;
        fn max_outfit_name_len(self: &OutfitService) -> u32;
        fn get_outfit_ptr(self: &mut OutfitService, name: &str) -> *mut Outfit;
        fn get_or_create_mut_outfit_ptr(self: &mut OutfitService, name: &str) -> *mut Outfit;
        fn add_outfit(self: &mut OutfitService, name: &str);
        fn current_outfit_ptr(self: &mut OutfitService, target: u32) -> *mut Outfit;
        fn has_outfit(self: &OutfitService, name: &str) -> bool;
        fn delete_outfit(self: &mut OutfitService, name: &str);
        fn set_favorite(self: &mut OutfitService, name: &str, favorite: bool);
        unsafe fn modify_outfit(self: &mut OutfitService, name: &str, add: &[*mut TESObjectARMO], remove: &[*mut TESObjectARMO], create_if_needed: bool);
        fn rename_outfit(self: &mut OutfitService, old_name: &str, new_name: &str) -> u32;
        fn set_outfit_c(self: &mut OutfitService, name: &str, target: u32);
        fn add_actor(self: &mut OutfitService, target: u32);
        fn remove_actor(self: &mut OutfitService, target: u32);
        fn list_actors(self: &OutfitService) -> Vec<u32>;
        fn set_location_based_switching_enabled(self: &mut OutfitService, setting: bool);
        fn set_outfit_using_location(self: &mut OutfitService, location: LocationType, target: u32);
        fn set_location_outfit(self: &mut OutfitService, location: LocationType, target: u32, name: &str);
        fn get_location_based_switching_enabled(self: &OutfitService) -> bool;
        fn unset_location_outfit(self: &mut OutfitService, location: LocationType, target: u32);
        fn get_location_outfit_name_c(self: &OutfitService, location: LocationType, target: u32) -> String;
        fn check_location_type_c(self: &OutfitService, keywords: Vec<String>, weather_flags: WeatherFlags, target: u32) -> OptionalLocationType;
        fn should_override(self: &OutfitService, target: u32) -> bool;
        fn get_outfit_names(self: &OutfitService, favorites_only: bool) -> Vec<String>;
        fn set_enabled(self: &mut OutfitService, option: bool);
        fn enabled_c(self: &OutfitService) -> bool;
        fn migration_save_v5(self: &mut OutfitService);
        fn migration_save_v6(self: &mut OutfitService);
        fn check_consistency(self: &mut OutfitService);
        fn save_json_c(self: &mut OutfitService) -> String;
        fn save_proto_c(self: &mut OutfitService) -> Vec<u8>;

        #[cxx_name = "RustOutfit"]
        type Outfit;
        unsafe fn conflicts_with(self: &Outfit, armor: *mut TESObjectARMO) -> bool;
        unsafe fn compute_display_set_c(self: &Outfit, equipped: &[*mut TESObjectARMO]) -> Vec<TESObjectARMOPtr>;
        fn set_slot_policy_c(self: &mut Outfit, slot: BIPED_OBJECT, policy: OptionalPolicy);
        fn set_blanket_slot_policy(self: &mut Outfit, policy: Policy);
        fn reset_to_default_slot_policy(self: &mut Outfit);
        fn armors_c(self: &Outfit) -> Vec<TESObjectARMOPtr>;
        fn favorite_c(self: &Outfit) -> bool;
        fn name_c(self: &Outfit) -> String;
        unsafe fn insert_armor(self: &mut Outfit, armor: *mut TESObjectARMO);
        unsafe fn erase_armor(self: &mut Outfit, armor: *mut TESObjectARMO);
        fn erase_all_armors(self: &mut Outfit);
        fn policy_names_for_outfit(self: &Outfit) -> Vec<String>;
    }
}
fn make_rust_string() -> *mut String {
    Box::leak(Box::new(String::from("asdf")))
}
fn outfit_service_create() -> Box<OutfitService> {
    Box::new(OutfitService::new())
}


impl ffi::Policy {
    fn policy_str(&self) -> Option<&'static str> {
        match *self {
            Self::XXXX => Some("XXXX"),
            Self::XXXE => Some("XXXE"),
            Self::XXXO => Some("XXXO"),
            Self::XXOX => Some("XXOX"),
            Self::XXOE => Some("XXOE"),
            Self::XXOO => Some("XXOO"),
            Self::XEXX => Some("XEXX"),
            Self::XEXE => Some("XEXE"),
            Self::XEXO => Some("XEXO"),
            Self::XEOX => Some("XEOX"),
            Self::XEOE => Some("XEOE"),
            Self::XEOO => Some("XEOO"),
            _ => None
        }
    }

    pub fn select(&self, has_equipped: bool, has_outfit: bool) -> Option<PolicySelection> {
        let policy_str = self.policy_str()?;
        let code = match (has_equipped, has_outfit) {
            (false, false) => policy_str.chars().nth(0),
            (true, false) => policy_str.chars().nth(1),
            (false, true) => policy_str.chars().nth(2),
            (true, true) => policy_str.chars().nth(3),
        };
        match code {
            Some('E') => Some(PolicySelection::Equipped),
            Some('O') => Some(PolicySelection::Outfit),
            _ => None
        }
    }

    fn translation_key(&self) -> String {
        return "$SkyOutSys_Desc_EasyPolicyName_".to_owned() + self.policy_str().unwrap_or_else(|| "");
    }

    pub const MAX: u8 = (Self::XEOO.repr + 1);
}

pub enum PolicySelection {
    Outfit,
    Equipped,
}

pub type FormID = u32;
