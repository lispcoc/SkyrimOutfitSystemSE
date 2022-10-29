#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(improper_ctypes)]

use cxx::{ExternType, type_id};

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

unsafe impl ExternType for RE_TESObjectARMO {
    type Id = type_id!("RE::TESObjectARMO");
    type Kind = cxx::kind::Opaque;
}

unsafe impl ExternType for SKSE_SerializationInterface {
    type Id = type_id!("SKSE::SerializationInterface");
    type Kind = cxx::kind::Opaque;
}

pub type RE_ActorFormID = RE_FormID;

pub type RE_TESObjectARMOFormID = RE_FormID;

pub fn REL_Relocate<T>(se_and_vr: T, ae: T) -> T {
    if unsafe { REL_Module::IsAE() } { ae } else { se_and_vr }
}

impl RE_TESObjectARMO {
    pub fn assign_using_mask(&mut self, dest: &mut [*mut RE_TESObjectARMO; RE_BIPED_OBJECTS_BIPED_OBJECT_kEditorTotal as usize]) {
        let mask = unsafe { self._base_10.GetSlotMask() };
        for slot in 0..RE_BIPED_OBJECTS_BIPED_OBJECT_kEditorTotal {
            if mask & (1 << slot) != 0 {
                dest[slot as usize] = self;
            }
        }
    }
}

impl SKSE_PluginVersionData {
    pub const fn default() -> Self {
        Self {
            dataVersion: SKSE_PluginVersionData_kVersion as u32,
            pluginVersion: 0,
            pluginName: [0; 256],
            author: [0; 256],
            supportEmail: [0; 252],
            _bitfield_align_1: [0; 0],
            _bitfield_1: __BindgenBitfieldUnit::<[u8; 1]>::new([0]),
            padding2: 0,
            padding3: 0,
            _bitfield_align_2: [0; 0],
            _bitfield_2: __BindgenBitfieldUnit::<[u8; 1]>::new([0]),
            padding5: 0,
            padding6: 0,
            compatibleVersions: [0; 16],
            xseMinimum: 0,
        }
    }
}