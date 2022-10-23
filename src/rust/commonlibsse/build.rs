fn main() {
    let includes = std::env::var("INCLUDE_PATHS").expect("No value for INCLUDE_PATHS");
    let defines = std::env::var("DEFINITIONS").expect("No value for DEFINITIONS");
    let options = std::env::var("OPTIONS").expect("No value for OPTIONS");

    let mut cxx = cxx_build::bridge("src/lib.rs");

    cxx
        .static_crt(true)
        .no_default_flags(true);

    cxx
        .include(includes.clone())
        .flag("/std:c++20")
        .flag("/EHsc");

    for option in options.split(" ") {
        cxx.flag(option);
    }

    for define in defines.split(" ") {
        let pair = define.split_once("=").expect("Failed to split at =");
        cxx.define(pair.0, Some(pair.1));
    }

    cxx
        .compile("commonlibsse-rs");
}
