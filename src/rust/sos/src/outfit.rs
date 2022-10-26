use std::collections::{BTreeMap, HashMap, HashSet};
use uncased::{Uncased, UncasedStr};
use commonlibsse::{
    RE_ActorFormID,
    SKSE_SerializationInterface,
    RE_BIPED_OBJECT,
    RE_BIPED_OBJECTS_BIPED_OBJECT_kEditorTotal,
    RE_ResolveARMOFormID,
    RE_TESObjectARMO,
    RE_PlayerCharacter_GetSingleton
};
use crate::{
    UncasedString,
    is_form_id_permitted,
    ffi::{
        OptionalLocationType,
        OptionalPolicy,
        TESObjectARMOPtr
    }
};
pub use crate::ffi::{LocationType, WeatherFlags};
use slot_policy::{Policies, Policy};

pub struct Outfit {
    pub name: UncasedString,
    pub armors: HashSet<*mut RE_TESObjectARMO>,
    pub favorite: bool,
    pub slot_policies: Policies,
}

impl Outfit {
    fn new(name: &str) -> Self {
        Outfit {
            name: Uncased::new(name).into_owned(),
            armors: Default::default(),
            favorite: false,
            slot_policies: Policies::standard()
        }
    }

    fn from_proto_data(input: &protos::outfit::Outfit, infc: &SKSE_SerializationInterface) -> Self {
        let mut outfit = Outfit {
            name: Uncased::new(input.name.as_str()).into_owned(),
            armors: Default::default(),
            favorite: input.is_favorite,
            slot_policies: Policies {
                slot_policies: Default::default(),
                blanket_slot_policy: Policy::XXXX
            }
        };
        for armor in &input.armors {
            let mut form_id = 0;
            if unsafe { (*infc).ResolveFormID(*armor, &mut form_id) } {
                let armor = unsafe { RE_ResolveARMOFormID(form_id) };
                if !armor.is_null() {
                    outfit.armors.insert(armor);
                }
            }
        }
        for (slot, policy) in &input.slot_policies {
            let policy = *policy as u8;
            if *slot >= RE_BIPED_OBJECTS_BIPED_OBJECT_kEditorTotal || policy >= Policy::MAX {
                continue
            }
            let policy = Policy { repr: policy };
            outfit.slot_policies.slot_policies.insert(*slot, policy);
        }
        if (input.slot_policy as u8) < Policy::MAX {
            outfit.slot_policies.blanket_slot_policy = Policy { repr: input.slot_policy as u8 };
        }
        outfit
    }

    pub fn armors_c(&self) -> Vec<TESObjectARMOPtr> {
        self.armors.iter().cloned().map(|ptr| TESObjectARMOPtr { ptr }).collect()
    }

    pub fn favorite_c(&self) -> bool {
        self.favorite
    }

    pub fn name_c(&self) -> String {
        self.name.clone().to_string()
    }

    pub unsafe fn insert_armor(&mut self, armor: *mut RE_TESObjectARMO) {
        self.armors.insert(armor);
    }

    pub unsafe fn erase_armor(&mut self, armor: *mut RE_TESObjectARMO) {
        self.armors.remove(&armor);
    }

    pub fn erase_all_armors(&mut self) {
        self.armors.clear();
    }

    pub unsafe fn conflicts_with(&self, armor: *mut RE_TESObjectARMO) -> bool {
        if armor.is_null() { return false }
        let mask = (*armor)._base_10.GetSlotMask();
        for armor in &self.armors {
            if !armor.is_null() && (mask & (**armor)._base_10.GetSlotMask()) != 0 {
                return true;
            }
        }
        false
    }
    pub unsafe fn compute_display_set_c(&self, equipped: &[*mut RE_TESObjectARMO]) -> Vec<TESObjectARMOPtr> {
        self.compute_display_set(equipped)
            .into_iter()
            .map(|ptr| TESObjectARMOPtr { ptr })
            .collect()
    }
    pub unsafe fn compute_display_set(&self, equipped: &[*mut RE_TESObjectARMO]) -> Vec<*mut RE_TESObjectARMO> {
        let equipped = {
            let mut slots = [std::ptr::null_mut(); RE_BIPED_OBJECTS_BIPED_OBJECT_kEditorTotal as usize];
            for armor in equipped {
                (**armor).assign_using_mask(&mut slots);
            }
            slots
        };
        let outfit = {
            let mut slots = [std::ptr::null_mut(); RE_BIPED_OBJECTS_BIPED_OBJECT_kEditorTotal as usize];
            for armor in &self.armors {
                (**armor).assign_using_mask(&mut slots);
            }
            slots
        };
        let mut mask = 0;
        let mut results = HashSet::new();
        for slot in 0..RE_BIPED_OBJECTS_BIPED_OBJECT_kEditorTotal {
            if mask & (1 << slot) != 0 { continue }
            let policy = self.slot_policies
                .slot_policies.get(&slot)
                .unwrap_or_else(|| &self.slot_policies.blanket_slot_policy)
                .clone();
            let selection = policy.select(
                !equipped[slot as usize].is_null(),
                !outfit[slot as usize].is_null());
            let selected_armor = match selection {
                None => None,
                Some(PolicySelection::Equipped) => Some(equipped[slot as usize]),
                Some(PolicySelection::Outfit) => Some(outfit[slot as usize]),
            };
            if let Some(selected_armor) = selected_armor {
                mask |= (*selected_armor)._base_10.GetSlotMask();
                results.insert(selected_armor);
            } else {
                continue
            }
        }
        results.into_iter().collect()
    }

    pub fn set_slot_policy_c(&mut self, slot: RE_BIPED_OBJECT, policy: OptionalPolicy) {
        let policy = if policy.has_value {
            Some(policy.value)
        } else {
            None
        };
        self.set_slot_policy(slot, policy)
    }

    pub fn set_slot_policy(&mut self, slot: RE_BIPED_OBJECT, policy: Option<Policy>) {
        if let Some(policy) = policy {
            self.slot_policies.slot_policies.insert(slot, policy);
        } else {
            self.slot_policies.slot_policies.remove(&slot);
        };
    }

    pub fn set_blanket_slot_policy(&mut self, policy: Policy) {
        self.slot_policies.blanket_slot_policy = policy;
    }

    pub fn reset_to_default_slot_policy(&mut self) {
        self.slot_policies = Policies::standard();
    }

    pub fn policy_names_for_outfit(&self) -> Vec<String> {
        let mut out: Vec<_> = (0..RE_BIPED_OBJECTS_BIPED_OBJECT_kEditorTotal).map(|slot| {
            if let Some(policy) = self.slot_policies.slot_policies.get(&slot) {
                policy.translation_key()
            } else {
                "$SkyOutSys_Desc_PolicyName_INHERIT".to_string()
            }
        }).collect();
        out.push(self.slot_policies.blanket_slot_policy.translation_key());
        out
    }

    pub fn save(&self) -> protos::outfit::Outfit {
        let mut out = protos::outfit::Outfit::default();
        out.name = self.name.to_string();
        for armor in &self.armors {
            if !armor.is_null() {
                let form_id = unsafe { (**armor)._base._base._base.GetFormID() };
                out.armors.push(form_id);
            }
        }
        out.is_favorite = self.favorite;
        for (slot, policy) in &self.slot_policies.slot_policies {
            out.slot_policies.insert(*slot, policy.repr as u32);
        }
        out.slot_policy = self.slot_policies.blanket_slot_policy.repr as u32;
        out
    }
}

pub struct ActorAssignments {
    pub current: Option<UncasedString>,
    pub location_based: BTreeMap<LocationType, UncasedString>
}

impl Default for ActorAssignments {
    fn default() -> Self {
        ActorAssignments {
            current: None,
            location_based: Default::default()
        }
    }
}

pub struct OutfitService {
    pub enabled: bool,
    pub outfits: HashMap<UncasedString, Outfit>,
    pub actor_assignments: BTreeMap<RE_ActorFormID, ActorAssignments>,
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

    pub fn max_outfit_name_len(&self) -> u32 {
        256
    }
    pub fn replace_with_new(&mut self) {
        *self = Self::new();
    }

    pub unsafe fn replace_with_proto_data_ptr(self: &mut OutfitService,
                              data: &[u8],
                              intfc: *const SKSE_SerializationInterface) -> bool {
        self.replace_with_proto(data, &*intfc)
    }

    pub unsafe fn replace_with_json_data(self: &mut OutfitService,
                                              json: &str,
                                              intfc: *const SKSE_SerializationInterface) -> bool {
        self.replace_with_json(json, &*intfc)
    }

    pub fn replace_with_proto(self: &mut OutfitService,
                                         data: &[u8],
                                         intfc: &SKSE_SerializationInterface) -> bool {
        if let Some(proto_version) = Self::from_proto(data, intfc) {
            *self = proto_version;
            true
        } else {
            false
        }
    }

    pub fn replace_with_json(self: &mut OutfitService,
                              data: &str,
                              intfc: &SKSE_SerializationInterface) -> bool {
        if let Some(proto_version) = Self::from_json(data, intfc) {
            *self = proto_version;
            true
        } else {
            false
        }
    }

    pub fn from_proto(data: &[u8], infc: &SKSE_SerializationInterface) -> Option<Self> {
        use protobuf::Message;
        let input = protos::outfit::OutfitSystem::parse_from_bytes(data).ok()?;
        Self::from_proto_struct(input, infc)
    }

    pub fn from_json(data: &str, infc: &SKSE_SerializationInterface) -> Option<Self> {
        let input = protobuf_json_mapping::parse_from_str(data).ok()?;
        Self::from_proto_struct(input, infc)
    }

    pub fn from_proto_struct(input: protos::outfit::OutfitSystem, infc: &SKSE_SerializationInterface) -> Option<Self> {
        let mut new = OutfitService::new();
        new.enabled = input.enabled;
        let mut actor_assignments = BTreeMap::new();
        for (old_form_id, assignments) in &input.actor_outfit_assignments {
            let mut form_id = 0;
            if unsafe { !(*infc).ResolveFormID(*old_form_id, &mut form_id) } || form_id == 0 { continue }
            let mut assignments_out = ActorAssignments::default();
            assignments_out.current = if assignments.current_outfit_name.is_empty() {
                None
            } else {
                Some(Uncased::from(assignments.current_outfit_name.as_str()).into_owned())
            };
            for (location, assignment) in assignments.location_based_outfits.iter() {
                let value = if assignment.is_empty() {
                    continue
                } else {
                    Uncased::from(assignment.as_str()).into_owned()
                };
                let location = LocationType { repr: *location };
                assignments_out.location_based.insert(location, value);
            }
            actor_assignments.insert(form_id as u32, assignments_out);
        }
        new.actor_assignments = actor_assignments;
        for outfit in input.outfits {
            new.outfits.insert(Uncased::from(outfit.name.as_str()).into_owned(),
                               Outfit::from_proto_data(&outfit, infc));
        }
        new.location_switching_enabled = input.location_based_auto_switch_enabled;
        Some(new)
    }

    pub fn get_outfit_ptr(&mut self, name: &str) -> *mut Outfit {
        if let Some(reference) = self.get_mut_outfit(name) {
            reference
        } else {
            std::ptr::null_mut()
        }
    }
    pub fn get_outfit(&self, name: &str) -> Option<&Outfit> {
        self.outfits.get(UncasedStr::new(name))
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
    pub fn add_outfit(&mut self, name: &str) {
        if !self.outfits.contains_key(UncasedStr::new(name)) {
            self.outfits.insert(Uncased::from(name).into_owned(), Outfit::new(name));
        }
    }
    pub fn current_outfit_ptr(&mut self, target: u32) -> *mut Outfit {
        if let Some(outfit) = self.current_mut_outfit(target) { outfit } else { std::ptr::null_mut() }
    }
    pub fn current_outfit(&self, target: RE_ActorFormID) -> Option<&Outfit> {
        let outfit_name = self.actor_assignments.get(&target).and_then(|assn| assn.current.clone())?;
        self.get_outfit(outfit_name.as_str())
    }
    pub fn current_mut_outfit(&mut self, target: RE_ActorFormID) -> Option<&mut Outfit> {
        let outfit_name = self.actor_assignments.get(&target).and_then(|assn| assn.current.clone())?;
        self.get_mut_outfit(outfit_name.as_str())
    }
    pub fn has_outfit(&self, name: &str) -> bool {
        self.outfits.contains_key(&Uncased::from_borrowed(name))
    }
    pub fn delete_outfit(&mut self, name: &str) {
        self.outfits.remove(UncasedStr::new(name));
    }
    pub fn set_favorite(&mut self, name: &str, favorite: bool) {
        if let Some(outfit) = self.outfits.get_mut(UncasedStr::new(name)) {
            outfit.favorite = favorite
        }
    }
    pub fn modify_outfit(&mut self, name: &str, add: &[*mut RE_TESObjectARMO], remove: &[*mut RE_TESObjectARMO], create_if_needed: bool) {
        let outfit = if create_if_needed {
            Some(self.get_or_create_mut_outfit(name))
        } else {
            self.get_mut_outfit(name)
        };
        let outfit = if let Some(outfit) = outfit { outfit } else { return };
        for added in add {
            outfit.armors.insert(*added);
        }
        for removed in remove {
            outfit.armors.remove(removed);
        }
    }
    pub fn rename_outfit(&mut self, old_name: &str, new_name: &str) -> u32 {
        // Returns 0 on success, 1 if outfit not found, 2 if name already used.
        if self.outfits.contains_key(UncasedStr::new(new_name)) { return 1 };
        let mut entry = if let Some(entry) = self.outfits.remove(UncasedStr::new(old_name)) { entry } else { return 1; };
        entry.name = Uncased::from(new_name).into_owned();
        self.outfits.insert(entry.name.clone(), entry);
        return 0;
    }
    pub fn set_outfit_c(&mut self, name: &str, target: RE_ActorFormID) {
        let name = if name.is_empty() {
            None
        } else {
            Some(name)
        };
        self.set_outfit(name, target)
    }
    pub fn set_outfit(&mut self, mut name: Option<&str>, target: RE_ActorFormID) {
        if name.map_or(false, |name| name.is_empty()) {
            name = None;
        }
        self.actor_assignments.get_mut(&target).map(|assn| {
            assn.current = name.map(|name| Uncased::from(name).into_owned());
        });
    }
    pub fn add_actor(&mut self, target: RE_ActorFormID) {
        self.actor_assignments.entry(target).or_insert_with(|| ActorAssignments::default());
    }
    pub fn remove_actor(&mut self, target: RE_ActorFormID) {
        if unsafe { (*RE_PlayerCharacter_GetSingleton())._base._base._base._base.GetFormID() } == target { return }
        self.actor_assignments.remove(&target);
    }
    pub fn list_actors(&self) -> Vec<RE_ActorFormID> {
        self.actor_assignments.keys().cloned().collect()
    }
    pub fn set_location_based_switching_enabled(&mut self, setting: bool) {
        self.location_switching_enabled = setting
    }
    pub fn get_location_based_switching_enabled(&self) -> bool {
        self.location_switching_enabled
    }

    pub fn set_outfit_using_location(&mut self, location: LocationType, target: RE_ActorFormID) {
        self
            .actor_assignments
            .get_mut(&target)
            .and_then(|assn| assn.location_based.get(&location))
            .map(|sel| sel.to_string())
            .map(|selected| {
                self.set_outfit(Some(selected.as_str()), target)
            });
    }
    pub fn set_location_outfit(&mut self, location: LocationType, target: RE_ActorFormID, name: &str) {
        if name.is_empty() {
            self.unset_location_outfit(location, target);
            return
        }
        self
            .actor_assignments
            .get_mut(&target)
            .map(|assn| assn.location_based.insert(location, Uncased::from(name).into_owned()));
    }
    pub fn unset_location_outfit(&mut self, location: LocationType, target: RE_ActorFormID) {
        self
            .actor_assignments
            .get_mut(&target)
            .map(|assn| assn.location_based.remove(&location));
    }
    pub fn get_location_outfit_name_c(&self, location: LocationType, target: RE_ActorFormID) -> String {
        self.get_location_outfit_name(location, target).unwrap_or_else(|| String::new())
    }
    pub fn get_location_outfit_name(&self, location: LocationType, target: RE_ActorFormID) -> Option<String> {
        self
            .actor_assignments
            .get(&target)
            .and_then(|assn| assn.location_based.get(&location))
            .map(|name| name.to_string())
    }
    pub fn check_location_type_c(&self, keywords: Vec<String>, weather_flags: WeatherFlags, target: RE_ActorFormID) -> OptionalLocationType {
        if let Some(result) = self.check_location_type(keywords, weather_flags, target) {
            OptionalLocationType {
                has_value: true,
                value: result
            }
        } else {
            OptionalLocationType {
                has_value: false,
                value: LocationType::World
            }
        }
    }
    pub fn check_location_type(&self, keywords: Vec<String>, weather_flags: WeatherFlags, target: RE_ActorFormID) -> Option<LocationType> {
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

    pub fn should_override(&self, target: RE_ActorFormID) -> bool {
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
    pub fn enabled_c(self: &OutfitService) -> bool {
        self.enabled
    }
    pub fn save_json_c(&self) -> String {
        self.save_json().unwrap_or_else(|| String::new())
    }
    pub fn save_json(&self) -> Option<String> {
        Some(protobuf_json_mapping::print_to_string(&self.save()).ok()?)
    }
    pub fn save_proto_c(&self) -> Vec<u8> {
        self.save_proto().unwrap_or_else(|| Vec::new())
    }
    pub fn save_proto(&self) -> Option<Vec<u8>> {
        use protobuf::Message;
        Some(self.save().write_to_bytes().ok()?)
    }
    pub fn save(&self) -> protos::outfit::OutfitSystem {
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
            out.actor_outfit_assignments.insert(*actor_handle, out_assn);
        }
        for (_, outfit) in &self.outfits {
            out.outfits.push(outfit.save());
        }
        out.location_based_auto_switch_enabled = self.location_switching_enabled;
        out
    }

    // Migrations
    pub fn migration_save_v5(&mut self) {
        for outfit in self.outfits.values_mut() {
            outfit.reset_to_default_slot_policy();
        }
    }

    pub fn migration_save_v6(&mut self) {
        self.actor_assignments.clear();
    }

    pub fn check_consistency(&mut self) {
        if !self.actor_assignments.contains_key(&unsafe { (*RE_PlayerCharacter_GetSingleton())._base._base._base._base.GetFormID() }) {
            self.actor_assignments.insert(unsafe { (*RE_PlayerCharacter_GetSingleton())._base._base._base._base.GetFormID() }, ActorAssignments::default());
        }
        self.actor_assignments.retain(|item, _| is_form_id_permitted(*item));
    }
}

pub mod slot_policy {
    use std::collections::BTreeMap;
    use commonlibsse::RE_BIPED_OBJECT;
    pub use crate::ffi::Policy;

    pub struct Policies {
        pub slot_policies: BTreeMap<RE_BIPED_OBJECT, Policy>,
        pub blanket_slot_policy: Policy,
    }

    impl Policies {
        pub fn standard() -> Self {
            use commonlibsse::RE_BIPED_OBJECTS_BIPED_OBJECT_kShield;
            let mut policies: BTreeMap<RE_BIPED_OBJECT, Policy> = Default::default();
            policies.insert(RE_BIPED_OBJECTS_BIPED_OBJECT_kShield, Policy::XEXO);
            Policies {
                slot_policies: policies,
                blanket_slot_policy: Policy::XXOO
            }
        }
    }
}

impl Policy {
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
