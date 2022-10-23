fn main() {
    let header_out = std::env::var("HEADER_GEN").expect("No value for HEADER_GEN");
    let crate_dir = std::env::var("CARGO_MANIFEST_DIR").unwrap();
    cbindgen::Builder::new()
        .with_crate(crate_dir)
        .generate()
        .expect("Unable to generate bindings")
        .write_to_file(header_out + "bindings.h.tmp");
}
