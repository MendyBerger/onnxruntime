wasmtime run --dir=. build_wasi/ort-wasi-simd.wasm


python3 tools/python/convert_onnx_models_to_ort.py squeezenet.onnx --output_dir .
