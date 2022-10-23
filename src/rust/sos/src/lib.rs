#![allow(dead_code)]

mod outfit;

use std::ffi::c_void;
#[allow(unused_imports)]
use protos::outfit as ProtoOutfit;

#[no_mangle]
pub extern fn bbbb() -> *mut c_void {
    commonlibsse::get_player_singleton().cast()
}
