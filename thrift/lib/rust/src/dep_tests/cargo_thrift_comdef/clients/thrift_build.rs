// @generated by autocargo

use std::env;
use std::fs;
use std::path::Path;
use thrift_compiler::Config;
use thrift_compiler::GenContext;
const CRATEMAP: &str = "\
thrift/lib/rust/src/dep_tests/test_competing_defaults.thrift crate //thrift/lib/rust/src/dep_tests:test_competing_defaults_if-rust
";
#[rustfmt::skip]
fn main() {
    println!("cargo:rerun-if-changed=thrift_build.rs");
    let out_dir = env::var_os("OUT_DIR").expect("OUT_DIR env not provided");
    let cratemap_path = Path::new(&out_dir).join("cratemap");
    fs::write(cratemap_path, CRATEMAP).expect("Failed to write cratemap");
    Config::from_env(GenContext::Clients)
        .expect("Failed to instantiate thrift_compiler::Config")
        .base_path("../../../../../../..")
        .types_crate("test_competing_defaults_if__types")
        .clients_crate("test_competing_defaults_if__clients")
        .run(["../../test_competing_defaults.thrift"])
        .expect("Failed while running thrift compilation");
}
