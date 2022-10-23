use std::ffi::c_void;
use cpp::cpp;

cpp!{{
    #include "RE/Skyrim.h"
}}

pub fn get_player_singleton() -> *mut Actor {
    unsafe {
        cpp!([] -> *mut Actor as "void*" {
            return RE::PlayerCharacter::GetSingleton();
        })
    }
}

macro_rules! skyrim_pointer {
    ($func_name:ident) => {
        #[repr(transparent)]
        #[derive(Debug, Copy, Clone)]
        pub struct $func_name {
            #[allow(dead_code)]
            ptr: *mut c_void,
        }
    }
}

skyrim_pointer!(Armor);
impl Armor {
    pub unsafe fn get_slot_mask(&self) -> u32 {
        let ptr = self.ptr;
        cpp!([ptr as "RE::TESObjectARMO const *"] -> u32 as "std::uint32_t" {
            return static_cast<std::uint32_t>(ptr->GetSlotMask());
        })
    }
}

skyrim_pointer!(Actor);
