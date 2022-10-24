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
        has_value: bool,
        value: LocationType,
    }

    #[namespace = "RE"]
    extern "C++" {
        include!("sos/include/customize.h");
        #[namespace = "SKSE"]
        type SerializationInterface = commonlibsse::SerializationInterface;
        type TESObjectARMO = commonlibsse::TESObjectARMO;
    }
    extern "Rust" {
        type OutfitService;
        fn outfit_service_create() -> Box<OutfitService>;
        unsafe fn replace_with_proto_ptr(self: &mut OutfitService, data: &[u8], intfc: *mut SerializationInterface) -> bool;
        fn get_outfit_ptr(self: &mut OutfitService, name: &str) -> *mut Outfit;
        fn get_or_create_mut_outfit_ptr(self: &mut OutfitService, name: &str) -> *mut Outfit;
        fn add_outfit(self: &mut OutfitService, name: &str);
        fn current_outfit_ptr(self: &mut OutfitService, target: u32) -> *mut Outfit;
        fn has_outfit(self: &OutfitService, name: &str) -> bool;
        fn delete_outfit(self: &mut OutfitService, name: &str);
        fn set_favorite(self: &mut OutfitService, name: &str, favorite: bool);
        fn modify_outfit(self: &mut OutfitService, name: &str, add: &[*mut TESObjectARMO], remove: &[*mut TESObjectARMO], create_if_needed: bool);
        fn rename_outfit(self: &mut OutfitService, old_name: &str, new_name: &str) -> u32;
        fn set_outfit_c(self: &mut OutfitService, name: &str, target: u32);
        fn add_actor(self: &mut OutfitService, target: u32);
        fn remove_actor(self: &mut OutfitService, target: u32);
        fn list_actors(self: &OutfitService) -> Vec<u32>;
        fn set_location_based_switching_enabled(self: &mut OutfitService, setting: bool);
        fn set_outfit_using_location(self: &mut OutfitService, location: LocationType, target: u32);
        fn set_location_outfit(self: &mut OutfitService, location: LocationType, target: u32, name: &str);
        fn unset_location_outfit(self: &mut OutfitService, location: LocationType, target: u32);
        fn get_location_outfit_name_c(self: &OutfitService, location: LocationType, target: u32) -> String;
        fn check_location_type_c(self: &OutfitService, keywords: Vec<String>, weather_flags: WeatherFlags, target: u32) -> OptionalLocationType;
        fn should_override(self: &OutfitService, target: u32) -> bool;
        fn get_outfit_names(self: &OutfitService, favorites_only: bool) -> Vec<String>;
        fn set_enabled(self: &mut OutfitService, option: bool);
        fn save_c(self: &mut OutfitService) -> Vec<u8>;

        #[cxx_name = "RustOutfit"]
        type Outfit;
        fn make_rust_string() -> *mut String;
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

    pub const MAX: u8 = (Self::XEOO.repr + 1);
}

pub enum PolicySelection {
    Outfit,
    Equipped,
}

pub type RawActorHandle = u32;
