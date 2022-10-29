use log::{set_logger, set_max_level, Level, Log, Metadata, Record};
use std::ffi::{c_char, c_int, CString};

pub struct SimpleLogger {}

extern "C" {
    fn RustLog(
        filename_in: *const c_char,
        line_in: c_int,
        funcname_in: *const c_char,
        level: c_int,
        message: *const c_char,
    );
    fn RustGetLogLevel() -> c_int;
}

#[allow(dead_code)]
mod levels {
    use std::ffi::c_int;
    pub const TRACE: c_int = 0;
    pub const DEBUG: c_int = 1;
    pub const INFO: c_int = 2;
    pub const WARN: c_int = 3;
    pub const ERR: c_int = 4;
    pub const CRITICAL: c_int = 5;
    pub const OFF: c_int = 6;
}

impl log::Log for SimpleLogger {
    fn enabled(&self, metadata: &Metadata) -> bool {
        let selected = match unsafe { RustGetLogLevel() } {
            levels::TRACE => Level::Trace,
            levels::DEBUG => Level::Debug,
            levels::INFO => Level::Info,
            levels::WARN => Level::Warn,
            levels::ERR => Level::Error,
            levels::CRITICAL => Level::Error,
            _ => return false,
        };
        metadata.level() <= selected
    }

    fn log(&self, record: &Record) {
        self.inner(record);
    }

    fn flush(&self) {}
}

impl SimpleLogger {
    pub fn setup() {
        let logger = Box::new(SimpleLogger {});
        set_logger(Box::leak(logger)).ok();
        Self::update_max_level();
    }
    pub fn update_max_level() {
        let selected = match unsafe { RustGetLogLevel() } {
            levels::TRACE => Level::Trace,
            levels::DEBUG => Level::Debug,
            levels::INFO => Level::Info,
            levels::WARN => Level::Warn,
            levels::ERR => Level::Error,
            levels::CRITICAL => Level::Error,
            _ => Level::Error,
        };
        set_max_level(selected.to_level_filter());
    }
    fn inner(&self, record: &Record) -> Option<()> {
        if self.enabled(record.metadata()) {
            let level = match record.level() {
                Level::Trace => levels::TRACE,
                Level::Debug => levels::DEBUG,
                Level::Info => levels::INFO,
                Level::Warn => levels::WARN,
                Level::Error => levels::ERR,
            };
            let fname = CString::new(record.file().unwrap_or_else(|| "N/A")).ok()?;
            let line = record.line().unwrap_or_else(|| 0);
            let func = CString::new(record.module_path().unwrap_or_else(|| "N/A")).ok()?;
            if let Some(static_msg) = record.args().as_str() {
                let message = CString::new(static_msg).ok()?;
                unsafe {
                    RustLog(
                        fname.as_ptr(),
                        line as c_int,
                        func.as_ptr(),
                        level,
                        message.as_ptr(),
                    )
                };
            } else {
                let message = CString::new(record.args().to_string()).ok()?;
                unsafe {
                    RustLog(
                        fname.as_ptr(),
                        line as c_int,
                        func.as_ptr(),
                        level,
                        message.as_ptr(),
                    )
                };
            };
            println!("{} - {}", record.level(), record.args());
        }
        Some(())
    }
}
