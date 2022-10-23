use cpp::cpp;
use std::pin::Pin;

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

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct Actor {
    _unused: [u8; 0],
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct Armor {
    _unused: [u8; 0],
}

impl Armor {
    pub fn get_slot_mask(&self) -> u32 {
        unsafe {
            cpp!([self as "RE::TESObjectARMO const *"] -> u32 as "std::uint32_t" {
                return static_cast<std::uint32_t>(self->GetSlotMask());
            })
        }
    }
}
