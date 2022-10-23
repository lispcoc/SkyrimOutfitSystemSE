use std::collections::{BTreeMap, HashMap, HashSet};
use commonlibsse::{Armor};

pub struct Outfit {
    pub name: String,
    pub armors: HashSet<*mut Armor>,
    pub favorite: bool,
    pub slot_policies: slot_policy::Policies,
}

impl Outfit {
    fn new(name: String) -> Self {
        Outfit {
            name,
            armors: Default::default(),
            favorite: false,
            slot_policies: slot_policy::Policies::standard()
        }
    }
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

impl OutfitService {
    fn get_outfit(&self, name: &str) -> Option<&Outfit> {
        self.outfits.get(name)
    }
    fn get_mut_outfit(&mut self, name: &str) -> Option<&mut Outfit> {
        self.outfits.get_mut(name)
    }
    fn get_or_create_outfit(&mut self, name: &str) -> &Outfit {
        self.outfits.entry(name.to_string()).or_insert_with(|| Outfit::new(name.to_string()))
    }
    fn get_or_create_mut_outfit(&mut self, name: &str) -> &mut Outfit {
        self.outfits.entry(name.to_string()).or_insert_with(|| Outfit::new(name.to_string()))
    }
    fn add_outfit(&mut self, name: &str) -> &Outfit {
        if !self.outfits.contains_key(name) {
            self.outfits.insert(name.to_string(), Outfit::new(name.to_string()));
        }
        self.outfits.get(name).unwrap()
    }
    fn add_mut_outfit(&mut self, name: &str) -> &mut Outfit {
        if !self.outfits.contains_key(name) {
            self.outfits.insert(name.to_string(), Outfit::new(name.to_string()));
        }
        self.outfits.get_mut(name).unwrap()
    }
    fn current_outfit(&self, target: RawActorHandle) -> Option<&Outfit> {
        let outfit_name = self.actor_assignments.get(&target).and_then(|assn| Some(assn.current.as_str()))?;
        self.get_outfit(outfit_name)
    }
    fn current_mut_outfit(&mut self, target: RawActorHandle) -> Option<&mut Outfit> {
        let outfit_name = self.actor_assignments.get(&target).and_then(|assn| Some(assn.current.as_str()))?.to_string();
        self.get_mut_outfit(&outfit_name)
    }
    fn has_outfit(&self, name: &str) -> bool {
        self.outfits.contains_key(name)
    }
}

#[repr(u32)]
#[derive(Ord, PartialOrd, Eq, PartialEq)]
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

pub mod slot_policy {
    use std::collections::BTreeMap;
    use super::BodySlot;

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

    pub struct Policies {
        pub slot_policies: BTreeMap<BodySlot, Policy>,
        pub blanket_slot_policy: Policy,
    }

    impl Policies {
        pub fn standard() -> Self {
            let mut policies: BTreeMap<BodySlot, Policy> = Default::default();
            policies.insert(BodySlot::Shield, Policy::XEXO);
            Policies {
                slot_policies: policies,
                blanket_slot_policy: Policy::XXOO
            }
        }
    }
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
