use crate::outfit_service_get_singleton_ptr;
use commonlibsse::*;
use log::*;
use std::ffi::c_void;

pub const UNIQUE_SIGNATURE: &[u8; 4] = b"cOft";
pub const UNIQUE_SIGNATURE_INT: u32 = signature_compute(UNIQUE_SIGNATURE);
pub const SIGNATURE: &[u8; 4] = b"AAOS";
pub const SIGNATURE_INT: u32 = signature_compute(SIGNATURE);
const fn signature_compute(value: &[u8; 4]) -> u32 {
    ((value[0] as u32) << 24)
        + ((value[1] as u32) << 16)
        + ((value[2] as u32) << 8)
        + ((value[3] as u32) << 0)
}
#[allow(dead_code)]
#[repr(u32)]
pub enum Versions {
    /// Unsupported handwritten binary format
    V1 = 1,
    /// Unsupported handwritten binary format
    V2 = 2,
    /// Unsupported handwritten binary format
    V3 = 3,
    /// First version with protobuf
    V4 = 4,
    /// First version with Slot Control System
    V5 = 5,
    /// Switch to FormID for Actor (instead of Handle)
    V6 = 6,
}

pub extern "C" fn serialization_save_callback(intfc: *mut SKSE_SerializationInterface) {
    let intfc = if !intfc.is_null() {
        unsafe { &mut *intfc }
    } else {
        error!("Save interface was NULL");
        return;
    };
    info!("Writing savedata...");
    if unsafe { intfc.OpenRecord(SIGNATURE_INT, Versions::V6 as u32) } {
        let service = unsafe { &mut *outfit_service_get_singleton_ptr() };
        let proto = if let Some(proto) = service.save_proto() {
            proto
        } else {
            error!("Could not serialize proto data!");
            return;
        };
        if unsafe { intfc.WriteRecordData(proto.as_ptr() as *const c_void, proto.len() as u32) } {
            info!("Saved successfully!")
        } else {
            error!("Failed to write proto to record data.")
        }
    }
}

pub extern "C" fn serialization_load_callback(intfc: *mut SKSE_SerializationInterface) {
    let intfc = if !intfc.is_null() {
        unsafe { &mut *intfc }
    } else {
        error!("Load interface was NULL");
        return;
    };
    let mut record_type = 0;
    let mut version = 0;
    let mut length = 0;
    while unsafe { intfc.GetNextRecordInfo(&mut record_type, &mut version, &mut length) } {
        match record_type {
            SIGNATURE_INT => {
                info!("Found matching signature.");
                let mut buffer = vec![0; length as usize];
                let read = unsafe {
                    intfc.ReadRecordData(buffer.as_mut_ptr() as *mut c_void, buffer.len() as u32)
                };
                if read != length {
                    error!("Did not read the correct amount of data for deserialization");
                    break;
                }
                let service = unsafe { &mut *outfit_service_get_singleton_ptr() };
                if !service.replace_with_proto(&buffer, intfc) {
                    error!("Failed to use proto data to instantiate");
                    break;
                };
                if version == Versions::V4 as u32 {
                    info!("Migrating outfit slot settings");
                    service.migration_save_v5();
                }
                if version < Versions::V6 as u32 {
                    info!("Migrating actor handles to FormID");
                    service.migration_save_v6();
                }
                service.check_consistency();
            }
            unknown => {
                error!("Unknown type code: {}", unknown);
                break;
            }
        }
    }
}
