extern crate cpp_build;
fn main() {
    let includes = std::env::var("INCLUDE_PATHS").expect("No value for INCLUDE_PATHS");
    let defines = std::env::var("DEFINITIONS").expect("No value for DEFINITIONS");
    let options = std::env::var("OPTIONS").expect("No value for OPTIONS");
    let header_out = std::env::var("HEADER_GEN").expect("No value for HEADER_GEN");

    let mut config = cpp_build::Config::new();

    config
        .include(includes)
        .flag_if_supported("/std:c++20")
        .flag_if_supported("/EHsc");

    for option in options.split(" ") {
        config.flag_if_supported(option);
    }

    for define in defines.split(" ") {
        let pair = define.split_once("=").expect("Failed to split at =");
        config.define(pair.0, Some(pair.1));
    }

    let profile = std::env::var("PROFILE").unwrap();

    config
        .flag(if profile == "debug" { "-MTd" } else { "-MT" })
        .build("src/lib.rs");

    let crate_dir = std::env::var("CARGO_MANIFEST_DIR").unwrap();

    cbindgen::Builder::new()
        .with_crate(crate_dir)
        .generate()
        .expect("Unable to generate bindings")
        .write_to_file(header_out + "bindings.h.tmp");
}
