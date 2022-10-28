use std::path::PathBuf;

fn main() {
    // Tell cargo to invalidate the built crate whenever the wrapper changes
    println!("cargo:rerun-if-changed=include/customize.hpp");

    let includes = std::env::var("INCLUDE_PATHS").expect("No value for INCLUDE_PATHS");
    let defines = std::env::var("DEFINITIONS").expect("No value for DEFINITIONS");

    let mut bindings = bindgen::Builder::default()
        .header("include/customize.hpp")
        .clang_arg("-DRUST_DEFINES")
        .clang_arg("-D_CRT_USE_BUILTIN_OFFSETOF")
        .clang_arg("-std=c++20")
        .clang_arg("-fms-compatibility")
        .clang_arg("-fms-extensions")
        .clang_arg("-fdelayed-template-parsing")
        .clang_arg(format!("-I{}", includes))
        .opaque_type("std::.*")
        .opaque_type("SKSE::stl.*")
        .opaque_type("RE::BSTArray.*")
        .opaque_type("RE::NiT.*")
        .opaque_type("RE::BSTSingleton.*")
        .opaque_type("RE::BSTSmartPointer.*")
        .allowlist_file("include/customize.hpp")
        .allowlist_type("RE::TESObjectARMO")
        .allowlist_type("RE::PlayerCharacter")
        .allowlist_type("SKSE::SerializationInterface")
        .blocklist_function("RE::BSTSmallArrayHeapAllocator.*")
        .generate_inline_functions(true)
        .parse_callbacks(Box::new(bindgen::CargoCallbacks));

    for define in defines.split(" ") {
        let pair = define.split_once("=").expect("Failed to split at =");
        bindings = bindings.clang_arg(format!("-D{}={}", pair.0, pair.1));
    }

    let bindings = bindings
        .generate()
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from(std::env::var("OUT_DIR").unwrap());

    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
