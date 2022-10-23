mod outfit;

use std::ffi::c_void;
use protos::outfit as ProtoOutfit;
use commonlibsse::Actor;

#[no_mangle]
pub extern fn bbbb() -> *mut c_void {
    commonlibsse::get_player_singleton().cast()
}
