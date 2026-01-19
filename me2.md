cargo build --release --target wasm32-wasip2



wit-bindgen c hand-rolled/onnx-runtime.wit -w onnx-runtime-impl --out-dir hand-rolled/



export WASI_SDK_PATH=/media/mendyberger/USB-Card/wasi/playground-main/webgpu-native-wasi/wasi-sdk



wac plug ./rust-guest/target/wasm32-wasip2/release/rust_guest.wasm --plug ./build_wasi/ort-wasi-simd.wasm -o ./wacd.wasm


output is ./wacd.wasm
