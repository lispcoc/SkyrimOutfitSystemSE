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