use literal_cstr::c;
use std::ffi::CString;
use std::ptr::null_mut;
use winapi::um::processthreadsapi::ExitProcess;
use winapi::um::winuser::{MessageBoxA, MB_OK};

pub fn setup_panic() {
    std::panic::set_hook(Box::new(|info| {
        // The payload may either downcast to a &str or String. Try to get both then pick one.
        let string = if let Some(string) = info.payload().downcast_ref::<String>() {
            Some(string.as_str())
        } else if let Some(&string) = info.payload().downcast_ref::<&'static str>() {
            Some(string)
        } else {
            None
        };
        // If we got a string payload, then print the reason.
        match string {
            Some(message) => quick_msg_box(format!("PANIC: {}", message)),
            None => quick_msg_box("PANIC: Unknown reason."),
        }
        unsafe {
            ExitProcess(42);
        }
    }));
}

/// A helper function to quickly show a message box. If `msg` cannot be converted to a CString (due to NULL bytes), a default message is shown.
pub fn quick_msg_box<S: AsRef<str>>(msg: S) {
    let message = CString::new(msg.as_ref())
        .unwrap_or_else(|_| c!("<Could not render error message.>").to_owned());
    let title = c!("Skyrim Outfit System Critical Error");
    unsafe {
        MessageBoxA(null_mut(), message.as_ptr(), title.as_ptr(), MB_OK);
    }
}
