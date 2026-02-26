# Merge both config files
cat > /media/mendyberger/USB-Card/wasi/onnxruntime/trustmark_operators.config << 'EOF'
# Merged config for TrustMark encoder and decoder
ai.onnx;1;GlobalAveragePool
ai.onnx;11;Conv
ai.onnx;12;MaxPool
ai.onnx;13;Cast,Concat,Flatten,Gemm,Resize,Sigmoid,Tanh
ai.onnx;14;Add,Mul,Relu,Reshape
EOF

# Rebuild with merged config
./build_wasi_simple.sh \
  -Donnxruntime_MINIMAL_BUILD_CUSTOM_OPS_CONFIG=/media/mendyberger/USB-Card/wasi/onnxruntime/trustmark_operators.config


# ./build_wasi_simple.sh

cd rust-guest
cargo build --release --target wasm32-wasip2
cd ..

wac plug ./rust-guest/target/wasm32-wasip2/release/rust_guest.wasm --plug ./build_wasi/ort-wasi-simd.wasm -o ./wacd.wasm

echo "output is ./wacd.wasm"

cp ./wacd.wasm ~/Downloads/wacd.wasm
