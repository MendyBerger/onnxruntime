wasmtime run --dir=. build_wasi/ort-wasi-simd.wasm

python3 tools/python/convert_onnx_models_to_ort.py squeezenet.onnx --output_dir .

https://onnxruntime.ai/docs/build/custom.html#disable-exceptions

export WASI_SDK_PATH=/media/mendyberger/USB-Card/wasi/playground-main/webgpu-native-wasi/wasi-sdk
