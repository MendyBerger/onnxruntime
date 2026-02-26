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
    let buffer = include_bytes!("./ghost_256.png");
    let input = image::load_from_memory(buffer).unwrap();

    println!("Loaded image: {:?}x{:?}, color: {:?}",
        input.width(), input.height(), input.color());
    // Sample a few pixels
    let rgb = input.to_rgb8();
    println!("First pixel RGB: {:?}", rgb.get_pixel(0, 0));
    println!("Center pixel RGB: {:?}", rgb.get_pixel(128, 128));

    let watermark = "1011011110011000111111000000011111011111011100000110110110111".to_owned();
    let encoded = tm.encode(watermark.clone(), input, 0.95).unwrap();
    // encoded.to_rgba8().save("./test.png").unwrap();
    // // let input = image::load_from_memory(encoded.as_ref()).unwrap(); //image::open("./test.png").unwrap();
    // let input = image::open("./test.png").unwrap();
    // let decoded = tm.decode(input).unwrap();
    let decoded = tm.decode(encoded).unwrap();
    assert_eq!(watermark, decoded);
    println!("Decoded watermark: {}", decoded);
    println!("Watermark: {}", watermark);
    println!("Success!!!!");
}
