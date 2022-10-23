use std::collections::{BTreeMap, HashMap, HashSet};
use commonlibsse::{Actor, Armor};

pub struct Outfit {
    pub name: String,
    pub armors: HashSet<*mut Armor>,
    pub favorite: bool,
    pub slot_policies: BTreeMap<BodySlot, Mode>,
    pub blanket_slot_policy: Mode,
}

pub struct ActorAssignments {
    pub current: String,
    pub location_based: BTreeMap<LocationType, String>
}

pub struct OutfitService {
    pub enabled: bool,
    pub outfits: HashMap<String, Outfit>,
    pub actor_assignments: BTreeMap<RawActorHandle, ActorAssignments>,
    pub location_switching_enabled: bool,
}

#[repr(u32)]
pub enum BodySlot {
    Head = 0,
    Hair = 1,
    Body = 2,
    Hands = 3,
    Forearms = 4,
    Amulet = 5,
    Ring = 6,
    Feet = 7,
    Calves = 8,
    Shield = 9,
    Tail = 10,
    LongHair = 11,
    Circlet = 12,
    Ears = 13,
    ModMouth = 14,
    ModNeck = 15,
    ModChestPrimary = 16,
    ModBack = 17,
    ModMisc1 = 18,
    ModPelvisPrimary = 19,
    DecapitateHead = 20,
    Decapitate = 21,
    ModPelvisSecondary = 22,
    ModLegRight = 23,
    ModLegLeft = 24,
    ModFaceJewelry = 25,
    ModChestSecondary = 26,
    ModShoulder = 27,
    ModArmLeft = 28,
    ModArmRight = 29,
    ModMisc2 = 30,
    FX01 = 31,

    HandToHandMelee = 32,
    OneHandSword = 33,
    OneHandDagger = 34,
    OneHandAxe = 35,
    OneHandMace = 36,
    TwoHandMelee = 37,
    Bow = 38,
    Staff = 39,
    Crossbow = 40,
    Quiver = 41,

    Total = 42
}

#[repr(u8)]
pub enum Mode {
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

type RawActorHandle = u32;
