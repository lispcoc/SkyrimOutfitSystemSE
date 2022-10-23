use std::collections::{BTreeMap, HashMap, HashSet};
use std::ptr::null_mut;
use uncased::{Uncased, UncasedStr};
use commonlibsse::{BIPED_OBJECT, TESObjectARMO};
use crate::{PolicySelection, UncasedString};
use crate::ffi::{WeatherFlags, LocationType, Policy};
use crate::outfit::slot_policy::Policies;

pub struct Outfit {
    pub name: UncasedString,
    pub armors: HashSet<*mut TESObjectARMO>,
    pub favorite: bool,
    pub slot_policies: slot_policy::Policies,
}

impl Outfit {
    fn new(name: &str) -> Self {
        Outfit {
            name: Uncased::new(name).into_owned(),
            armors: Default::default(),
            favorite: false,
            slot_policies: slot_policy::Policies::standard()
        }
    }

    unsafe fn conflicts_with(&self, armor: *mut TESObjectARMO) -> bool {
        if armor.is_null() { return false }
        let mask = (*armor).GetSlotMask();
        for armor in &self.armors {
            if !armor.is_null() && (mask.repr & (**armor).GetSlotMask().repr) != 0 {
                return true;
            }
        }
        false
    }

    unsafe fn compute_display_set(&self, equipped: Vec<*mut TESObjectARMO>) -> Vec<*mut TESObjectARMO> {
        let equipped = {
            let mut slots = [null_mut(); BIPED_OBJECT::MAX_IN_GAME];
            for armor in equipped {
                (*armor).assign_using_mask(&mut slots);
            }
            slots
        };
        let outfit = {
            let mut slots = [null_mut(); BIPED_OBJECT::MAX_IN_GAME];
            for armor in &self.armors {
                (**armor).assign_using_mask(&mut slots);
            }
            slots
        };
        let mut mask = 0;
        let mut results = HashSet::new();
        for slot in 0..BIPED_OBJECT::MAX_IN_GAME {
            if mask & (1 << slot) != 0 { continue }
            let policy = self.slot_policies
                .slot_policies.get(&BIPED_OBJECT { repr: slot as u32 })
                .unwrap_or_else(|| &self.slot_policies.blanket_slot_policy)
                .clone();
            let selection = policy.select(!equipped[slot].is_null(), !outfit[slot].is_null());
            let selected_armor = match selection {
                None => None,
                Some(PolicySelection::Equipped) => Some(equipped[slot]),
                Some(PolicySelection::Outfit) => Some(outfit[slot]),
            };
            if let Some(selected_armor) = selected_armor {
                mask |= (*selected_armor).GetSlotMask().repr;
                results.insert(selected_armor);
            } else {
                continue
            }
        }
        results.into_iter().collect()
    }

    fn set_slot_policy(&mut self, slot: BIPED_OBJECT, policy: Option<Policy>) {
        if let Some(policy) = policy {
            self.slot_policies.slot_policies.insert(slot, policy);
        } else {
            self.slot_policies.slot_policies.remove(&slot);
        };
    }

    fn set_blanket_slot_policy(&mut self, policy: Policy) {
        self.slot_policies.blanket_slot_policy = policy;
    }

    fn reset_to_default_slot_policy(&mut self) {
        self.slot_policies = Policies::standard();
    }

    fn save(&self) -> protos::outfit::Outfit {
        let mut out = protos::outfit::Outfit::default();
        out.name = self.name.to_string();
        for armor in &self.armors {
            if !armor.is_null() {
                let form_id = unsafe { (**armor).GetFormID() };
                out.armors.push(form_id);
            }
        }
        out.is_favorite = self.favorite;
        for (slot, policy) in &self.slot_policies.slot_policies {
            out.slot_policies.insert(slot.repr, policy.repr as u32);
        }
        out.slot_policy = self.slot_policies.blanket_slot_policy.repr as u32;
        out
    }
}

pub struct ActorAssignments {
    pub current: Option<UncasedString>,
    pub location_based: BTreeMap<LocationType, UncasedString>
}


pub struct OutfitService {
    pub enabled: bool,
    pub outfits: HashMap<UncasedString, Outfit>,
    pub actor_assignments: BTreeMap<RawActorHandle, ActorAssignments>,
    pub location_switching_enabled: bool,
}

impl OutfitService {
    pub fn new() -> Self {
        OutfitService {
            enabled: false,
            outfits: Default::default(),
            actor_assignments: Default::default(),
            location_switching_enabled: false
        }
    }
    pub fn get_outfit_ptr(&mut self, name: &str) -> *mut Outfit {
        if let Some(reference) = self.get_mut_outfit(name) {
            reference
        } else {
            null_mut()
        }
    }
    pub fn get_outfit(&self, name: &str) -> Option<&Outfit> {
        let value = self.outfits.get(UncasedStr::new(name));
        value
    }
    pub fn get_mut_outfit(&mut self, name: &str) -> Option<&mut Outfit> {
        self.outfits.get_mut(UncasedStr::new(name))
    }
    pub fn get_or_create_outfit(&mut self, name: &str) -> &Outfit {
        self.outfits.entry(Uncased::from(name).into_owned()).or_insert_with(|| Outfit::new(name))
    }
    pub fn get_or_create_mut_outfit_ptr(&mut self, name: &str) -> *mut Outfit {
        self.get_or_create_mut_outfit(name)
    }
    pub fn get_or_create_mut_outfit(&mut self, name: &str) -> &mut Outfit {
        self.outfits.entry(Uncased::from(name).into_owned()).or_insert_with(|| Outfit::new(name))
    }
    pub fn add_outfit(&mut self, name: &str) -> &Outfit {
        if !self.outfits.contains_key(UncasedStr::new(name)) {
            self.outfits.insert(Uncased::from(name).into_owned(), Outfit::new(name));
        }
        self.outfits.get(UncasedStr::new(name)).unwrap()
    }
    pub fn add_mut_outfit(&mut self, name: &str) -> &mut Outfit {
        if !self.outfits.contains_key(UncasedStr::new(name)) {
            self.outfits.insert(Uncased::from(name).into_owned(), Outfit::new(name));
        }
        self.outfits.get_mut(UncasedStr::new(name)).unwrap()
    }
    pub fn current_outfit(&self, target: RawActorHandle) -> Option<&Outfit> {
        let outfit_name = self.actor_assignments.get(&target).and_then(|assn| assn.current.clone())?;
        self.get_outfit(outfit_name.as_str())
    }
    pub fn current_mut_outfit(&mut self, target: RawActorHandle) -> Option<&mut Outfit> {
        let outfit_name = self.actor_assignments.get(&target).and_then(|assn| assn.current.clone())?;
        self.get_mut_outfit(outfit_name.as_str())
    }
    pub fn has_outfit(&self, name: &str) -> bool {
        self.outfits.contains_key(&Uncased::from_borrowed(name))
    }
    pub fn delete_outfit(&mut self, name: &str) {
        self.outfits.remove(UncasedStr::new(name));
    }
    pub fn set_outfit(&mut self, mut name: Option<&str>, target: RawActorHandle) {
        if name.map_or(false, |name| name.is_empty()) {
            name = None;
        }
        self.actor_assignments.get_mut(&target).map(|assn| {
            assn.current = name.map(|name| Uncased::from(name).into_owned());
        });
    }
    pub fn add_actor(&mut self, target: RawActorHandle) {
        self.actor_assignments.entry(target).or_insert_with(|| ActorAssignments {
            current: None,
            location_based: Default::default()
        });
    }
    pub fn remove_actor(&mut self, target: RawActorHandle) {
        self.actor_assignments.remove(&target);
    }
    pub fn list_actors(&self) -> Vec<RawActorHandle> {
        self.actor_assignments.keys().cloned().collect()
    }
    pub fn set_location_based_switching_enabled(&mut self, setting: bool) {
        self.location_switching_enabled = setting
    }
    pub fn set_outfit_using_location(&mut self, location: LocationType, target: RawActorHandle) {
        self
            .actor_assignments
            .get_mut(&target)
            .and_then(|assn| assn.location_based.get(&location))
            .map(|sel| sel.to_string())
            .map(|selected| {
                self.set_outfit(Some(selected.as_str()), target)
            });
    }
    pub fn set_location_outfit(&mut self, location: LocationType, target: RawActorHandle, name: &str) {
        if name.is_empty() {
            self.unset_location_outfit(location, target);
            return
        }
        self
            .actor_assignments
            .get_mut(&target)
            .map(|assn| assn.location_based.insert(location, Uncased::from(name).into_owned()));
    }
    pub fn unset_location_outfit(&mut self, location: LocationType, target: RawActorHandle) {
        self
            .actor_assignments
            .get_mut(&target)
            .map(|assn| assn.location_based.remove(&location));
    }
    pub fn get_location_outfit_name(&self, location: LocationType, target: RawActorHandle) -> Option<String> {
        self
            .actor_assignments
            .get(&target)
            .and_then(|assn| assn.location_based.get(&location))
            .map(|name| name.to_string())
    }
    pub fn check_location_type(&self, keywords: Vec<String>, weather_flags: WeatherFlags, target: RawActorHandle) -> Option<LocationType> {
        let kw_map: HashSet<_> = keywords.into_iter().collect();
        let actor_assn = &self.actor_assignments.get(&target)?.location_based;
        macro_rules! check_location {
            ($variant:expr, $check_code:expr) => {
                if actor_assn.contains_key(&$variant) && ($check_code) {
                    return Some($variant)
                };
            }
        }

        check_location!(LocationType::CitySnowy, kw_map.contains("LocTypeCity") && weather_flags.snowy);
        check_location!(LocationType::CityRainy, kw_map.contains("LocTypeCity") && weather_flags.rainy);
        check_location!(LocationType::City, kw_map.contains("LocTypeCity"));

        // A city is considered a town, so it will use the town outfit unless a city one is selected.
        check_location!(LocationType::TownSnowy, kw_map.contains("LocTypeTown") && kw_map.contains("LocTypeCity") && weather_flags.snowy);
        check_location!(LocationType::TownRainy, kw_map.contains("LocTypeTown") && kw_map.contains("LocTypeCity") && weather_flags.rainy);
        check_location!(LocationType::Town, kw_map.contains("LocTypeTown") && kw_map.contains("LocTypeCity"));

        check_location!(LocationType::DungeonSnowy, kw_map.contains("LocTypeDungeon") && weather_flags.snowy);
        check_location!(LocationType::DungeonRainy, kw_map.contains("LocTypeDungeon") && weather_flags.rainy);
        check_location!(LocationType::Dungeon, kw_map.contains("LocTypeDungeon"));

        check_location!(LocationType::WorldSnowy, weather_flags.snowy);
        check_location!(LocationType::WorldRainy, weather_flags.rainy);
        check_location!(LocationType::World, true);

        None
    }

    pub fn should_override(&self, target: RawActorHandle) -> bool {
        if !self.enabled || self.actor_assignments.get(&target).and_then(|assn| assn.current.as_ref()) == None {
            false
        } else {
            true
        }
    }
    pub fn get_outfit_names(&self, favorites_only: bool) -> Vec<String> {
        self.outfits.values()
            .filter(|outfit| !favorites_only || outfit.favorite)
            .map(|outfit| outfit.name.to_string())
            .collect()
    }
    pub fn set_enabled(&mut self, option: bool) {
        self.enabled = option
    }

    pub fn save(&self) -> Option<Vec<u8>> {
        use protobuf::Message;
        let mut out = protos::outfit::OutfitSystem::default();
        out.enabled = self.enabled;
        for (actor_handle, assn) in &self.actor_assignments {
            let mut out_assn = protos::outfit::ActorOutfitAssignment::default();
            out_assn.current_outfit_name = assn.current
                .as_ref()
                .map(|s| s.to_string())
                .unwrap_or_else(|| String::new());
            out_assn.location_based_outfits = assn.location_based.iter()
                .map(|(loc, name)| {
                    (loc.repr, name.to_string())
                })
                .collect();
            out.actor_outfit_assignments.insert(*actor_handle as u64, out_assn);
        }
        for (_, outfit) in &self.outfits {
            out.outfits.push(outfit.save());
        }
        out.location_based_auto_switch_enabled = self.location_switching_enabled;
        Some(out.write_to_bytes().ok()?)
    }
}

pub mod slot_policy {
    use std::collections::BTreeMap;
    use commonlibsse::BIPED_OBJECT;
    use crate::ffi::Policy;

    pub struct Policies {
        pub slot_policies: BTreeMap<BIPED_OBJECT, Policy>,
        pub blanket_slot_policy: Policy,
    }

    impl Policies {
        pub fn standard() -> Self {
            let mut policies: BTreeMap<BIPED_OBJECT, Policy> = Default::default();
            policies.insert(BIPED_OBJECT::kShield, Policy::XEXO);
            Policies {
                slot_policies: policies,
                blanket_slot_policy: Policy::XXOO
            }
        }
    }
}

pub type RawActorHandle = u32;
