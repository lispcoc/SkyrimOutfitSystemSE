fn main() {
    protobuf_codegen::Codegen::new()
        .pure()
        // All inputs and imports from the inputs must reside in `includes` directories.
        .includes(&["../../protos"])
        // Inputs must reside in some of include paths.
        .input("../../protos/outfit.proto")
        // Specify output directory relative to Cargo output directory.
        .cargo_out_dir("protos")
        .run_from_script();
}
