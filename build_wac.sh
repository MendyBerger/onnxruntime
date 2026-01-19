./build_wasi_simple.sh

cd rust-guest
cargo build --release --target wasm32-wasip2
cd ..

wac plug ./rust-guest/target/wasm32-wasip2/release/rust_guest.wasm --plug ./build_wasi/ort-wasi-simd.wasm -o ./wacd.wasm

echo "output is ./wacd.wasm"
