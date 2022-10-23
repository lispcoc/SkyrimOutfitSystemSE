use cpp::cpp;

cpp!{{
    #include "RE/Skyrim.h"
}}

pub fn get_player_singleton() -> *mut Actor {
    let ptr = unsafe {
        cpp!([] -> *mut Actor as "void*" {
            return RE::PlayerCharacter::GetSingleton();
        })
    };
    ptr
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
