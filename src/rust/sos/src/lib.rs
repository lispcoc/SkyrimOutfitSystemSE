#![allow(dead_code)]

mod outfit;

use uncased::{Uncased};
#[allow(unused_imports)]
use protos::outfit as ProtoOutfit;
use commonlibsse::PlayerCharacter;
type UncasedString = Uncased<'static>;

#[cxx::bridge]
mod ffi {
    #[namespace = "RE"]
    extern "C++" {
        include!("sos/include/customize.h");
        type PlayerCharacter = commonlibsse::ffi::PlayerCharacter;
    }
    extern "Rust" {
        type Testing;
        fn create() -> Box<Testing>;
        fn bbbb(self: &Testing) -> *mut PlayerCharacter;
    }
}

struct Testing {
}

fn create() -> Box<Testing> {
    Box::new(Testing{})
}

impl Testing {
    fn bbbb(&self) -> *mut PlayerCharacter {
        commonlibsse::PlayerCharacter_GetSingleton()
    }
}