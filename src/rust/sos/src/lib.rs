#![allow(dead_code)]

mod outfit;

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

    #[namespace = "RE"]
    extern "C++" {
        include!("sos/include/customize.h");
        type PlayerCharacter = commonlibsse::ffi::PlayerCharacter;

    }
    extern "Rust" {
        type OutfitService;
        fn create() -> Box<OutfitService>;
        fn delete_outfit(self: &mut OutfitService, name: &str);
        fn get_outfit_ptr(self: &mut OutfitService, name: &str) -> *mut Outfit;
        fn get_or_create_mut_outfit_ptr(self: &mut OutfitService, name: &str) -> *mut Outfit;
        fn list_actors(self: &OutfitService) -> Vec<u32>;
        #[cxx_name = "RustOutfit"]
        type Outfit;
    }
}

fn create() -> Box<OutfitService> {
    Box::new(OutfitService::new())
}