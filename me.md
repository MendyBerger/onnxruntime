wasmtime run --dir=. build_wasi/ort-wasi-simd.wasm

python3 tools/python/convert_onnx_models_to_ort.py squeezenet.onnx --output_dir .

https://onnxruntime.ai/docs/build/custom.html#disable-exceptions

export WASI_SDK_PATH=/media/mendyberger/USB-Card/wasi/playground-main/webgpu-native-wasi/wasi-sdk



align webgpu versions (from onnxtunetime)
dawn 13c1635a14574ebb7116b56a69f5519301417fda (https://github.com/microsoft/onnxruntime/blob/06004826cc99dd8b8b92dbf000db3d3525716f22/cmake/deps.txt#L58)
webgpu-headers: c8b371dd2ff8a2b028fdc0206af5958521181ba8 (https://github.com/google/dawn/tree/13c1635a14574ebb7116b56a69f5519301417fda/third_party/webgpu-headers)
webgpu-spec: a2637f7b880c2556919cdb288fe89815e0ed1c41 (https://github.com/google/dawn/tree/13c1635a14574ebb7116b56a69f5519301417fda/third_party)




from wasi:webgpu spec:
webgpu-headers option 1: a2da5f3057228374aaa1df7e3d30c3ebcda8f951 (about same time as wasi:webgpu is pinned to)
webgpu-headers option 2: 60cd9020309b87a30cd7240aad32accd24262a5e (dawn was pinned to it at some point, a few months later https://github.com/google/dawn/commit/37ff63983f04b88637a5d3bf8074714d7127f2c5)
https://github.com/webgpu-native/webgpu-headers/compare/a2da5f3057228374aaa1df7e3d30c3ebcda8f951..60cd9020309b87a30cd7240aad32accd24262a5e
