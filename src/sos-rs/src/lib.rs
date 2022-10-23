mod outfit;
mod protos;

use std::ffi::c_void;
use cpp::cpp;
use crate::protos::outfit as ProtoOutfit;

cpp!{{
    #include "RE/Skyrim.h"
}}

#[no_mangle]
pub extern fn bbbb() -> *mut c_void {
    let ptr = unsafe {
        cpp!([] -> *mut c_void as "void*" {
            return RE::PlayerCharacter::GetSingleton();
        })
    };
    ptr
}

pub struct Armor;
pub struct Actor;