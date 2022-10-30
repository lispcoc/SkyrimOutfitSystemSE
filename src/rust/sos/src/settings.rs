use std::path::PathBuf;

use crate::interface::ffi::*;
use configparser::ini::Ini;
use lazy_static::lazy_static;
use log::*;

pub struct Settings {
    ini: Ini
}

impl Settings {
    pub fn new() -> Self {
        let mut file = PathBuf::from(GetRuntimeDirectory().to_string());
        file.push("Data\\SKSE\\Plugins\\SkyrimOutfitSystemSE.ini");
        let mut ini = Ini::new();
        if let Err(error) = ini.load(file.clone()) {
            warn!("Could not load INI file at {} due to {}", file.display(), error);
        } else {
            info!("Loaded INI file in {}", file.display());
        }
        Settings { ini }
    }

    pub fn extra_logging_enabled(&self) -> bool {
        self.ini.getboolcoerce("Debug", "ExtraLogging")
        .unwrap_or_else(|_| None)
        .unwrap_or_else(|| false)
    }
}

lazy_static! {
    pub static ref SETTINGS: Settings = Settings::new();
}