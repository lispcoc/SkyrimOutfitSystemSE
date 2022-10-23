#![allow(dead_code)]

mod outfit;

use std::ffi::c_void;
use uncased::{Uncased};
#[allow(unused_imports)]
use protos::outfit as ProtoOutfit;

type UncasedString = Uncased<'static>;

#[no_mangle]
pub extern fn bbbb() -> *mut c_void {
    commonlibsse::get_player_singleton().cast()
}
