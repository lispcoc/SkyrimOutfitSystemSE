use std::ffi::{CString};
use std::ptr::null_mut;
use literal_cstr::c;
use winapi::um::winuser::{MessageBoxA, MB_OK};
use winapi::um::processthreadsapi::ExitProcess;

pub fn setup_panic() {
    std::panic::set_hook(Box::new(|info| {
        // The payload may either downcast to a &str or String. Try to get both then pick one.
        let string = info.payload().downcast_ref::<String>();
        let str = info.payload().downcast_ref::<&str>();
        let str = match (string, str) {
            (Some(str), _) => Some(str.as_str()),
            (_, Some(str)) => Some(*str),
            (None, None) => None,
        };
        // If we got a string payload, then print the reason.
        match str {
            Some(message) => quick_msg_box(&format!("PANIC: {}", message)),
            None => quick_msg_box(&"PANIC: Unknown reason.".to_string()),
        }
        unsafe {
            ExitProcess(42);
        }
    }));
}

/// A helper function to quickly show a message box. Panics if `msg` cannot be converted to a [`WideCString`]
/// (likely, because it contains NULL bytes inside of it.
pub fn quick_msg_box(msg: &str) {
    let message = CString::new(msg).unwrap_or_default();
    let title = c!("Skyrim Outfit System Critical Error");
    unsafe {
        MessageBoxA(null_mut(), message.as_ptr(), title.as_ptr(), MB_OK);
    }
}
