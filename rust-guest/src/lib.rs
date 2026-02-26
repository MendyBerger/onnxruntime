wit_bindgen::generate!({
    world: "onnx-runtime-user",
    path: "../hand-rolled/onnx-runtime.wit",
});

mod lib2;
mod bits;
mod image_processing;
mod model;

use crate::cosmonic::onnx_runtime::types;
use lib2::*;

struct MyCliRunner;
impl wasi::exports::cli::run::Guest for MyCliRunner {
    fn run() -> Result<(), ()> {
        main();
        Ok(())
    }
}
wasi::cli::command::export!(MyCliRunner);

fn main() {
    let tm = Trustmark::new("./models", Variant::Q, Version::Bch5).unwrap();
    // let input = image::open(path.as_ref()).unwrap();
    let buffer = include_bytes!("./ghost.png");
    let input = image::load_from_memory(buffer).unwrap();
    let watermark = "1011011110011000111111000000011111011111011100000110110110111".to_owned();
    let encoded = tm.encode(watermark.clone(), input, 0.95).unwrap();
    encoded.to_rgba8().save("./test.png").unwrap();
    let input = image::open("./test.png").unwrap();
    let decoded = tm.decode(input).unwrap();
    assert_eq!(watermark, decoded);
}
