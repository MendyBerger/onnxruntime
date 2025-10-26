# WASI-SDK Quick Reference Card

## One-Time Setup

```bash
# 1. Download and install WASI-SDK
wget https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-22/wasi-sdk-22.0-linux.tar.gz
tar xzf wasi-sdk-22.0-linux.tar.gz

# 2. Set environment variable (add to ~/.bashrc for persistence)
export WASI_SDK_PATH="$(pwd)/wasi-sdk-22.0"
```

## Quick Build Commands

```bash
# Standard build (RECOMMENDED)
./build_wasi_simple.sh

# Debug build
./build_wasi_simple.sh -DCMAKE_BUILD_TYPE=Debug

# With relaxed SIMD (faster, less precise)
./build_wasi_simple.sh -Donnxruntime_ENABLE_WEBASSEMBLY_RELAXED_SIMD=ON

# Minimal build (smaller binary)
./build_wasi_simple.sh -Donnxruntime_MINIMAL_BUILD=ON
```

## Running the Output

```bash
# With wasmtime (RECOMMENDED)
wasmtime run --dir=. build_wasi/ort-wasi-simd.wasm

# With wasmer
wasmer run build_wasi/ort-wasi-simd.wasm

# In Node.js (requires @wasmer/wasi)
npm install @wasmer/wasi
node run_wasi.js  # See README_WASI.md for example
```

## Common Build Options

| Option | Value | Description |
|--------|-------|-------------|
| CMAKE_BUILD_TYPE | Debug/Release | Build mode |
| onnxruntime_ENABLE_WEBASSEMBLY_SIMD | ON/OFF | SIMD instructions |
| onnxruntime_ENABLE_WEBASSEMBLY_RELAXED_SIMD | ON/OFF | Relaxed SIMD |
| onnxruntime_USE_XNNPACK | ON/OFF | CPU optimization |
| onnxruntime_MINIMAL_BUILD | ON/OFF | Minimal binary |
| onnxruntime_ENABLE_TRAINING_APIS | ON/OFF | Training support |

## File Locations

- **Build script**: `build_wasi_simple.sh`
- **Toolchain file**: `cmake/wasi-sdk.cmake`
- **Output directory**: `build_wasi/`
- **Output binary**: `build_wasi/ort-wasi[-simd][-training].wasm`
- **Full docs**: `README_WASI.md`

## Troubleshooting Quick Fixes

```bash
# Problem: WASI_SDK_PATH not set
export WASI_SDK_PATH=/path/to/wasi-sdk-22.0

# Problem: Build fails
rm -rf build_wasi && ./build_wasi_simple.sh

# Problem: Can't find wasmtime
curl https://wasmtime.dev/install.sh -sSf | bash

# Problem: Slow performance
./build_wasi_simple.sh -Donnxruntime_ENABLE_WEBASSEMBLY_SIMD=ON
```

## Output Files

```
build_wasi/
├── ort-wasi.wasm              # Standard build
├── ort-wasi-simd.wasm         # With SIMD
├── ort-wasi-relaxedsimd.wasm  # With relaxed SIMD
└── ort-training-wasi.wasm     # With training APIs
```

## Key Differences from Emscripten

| Feature | WASI-SDK | Emscripten |
|---------|----------|------------|
| Output | .wasm | .js + .wasm |
| Size | Smaller | Larger (JS glue) |
| JSEP | ❌ No | ✅ Yes |
| WebGPU | ❌ No | ✅ Yes |
| Server-side | ✅ Excellent | ⚠️ Limited |
| Browser | ⚠️ With polyfill | ✅ Native |
| Runtimes | wasmtime, wasmer | Node.js, browsers |

## Supported Features

✅ Core ONNXRuntime
✅ SIMD (standard and relaxed)
✅ XNNPACK optimization
✅ Training APIs
✅ Multiple WASI runtimes
✅ Debug symbols
✅ Static library mode

## Not Supported

❌ JSEP
❌ WebGPU
❌ WebNN
❌ Traditional threading
❌ Asyncify/JSPI

## Help & Documentation

- **Full Guide**: `README_WASI.md`
- **Technical Details**: `WASI_SDK_MIGRATION.md`
- **Changes Summary**: `WASI_CHANGES_SUMMARY.md`
- **WASI-SDK Docs**: https://github.com/WebAssembly/wasi-sdk
- **Wasmtime Docs**: https://docs.wasmtime.dev/

## Example Integration (Node.js)

```javascript
const { init, WASI } = require('@wasmer/wasi');
const fs = require('fs');

(async () => {
    await init();
    const wasi = new WASI({ args: [], env: {} });
    const binary = fs.readFileSync('./ort-wasi.wasm');
    const module = await WebAssembly.compile(binary);
    const instance = await WebAssembly.instantiate(module, {
        ...wasi.getImports(module)
    });
    wasi.start(instance);
})();
```

## Getting Help

1. Check `README_WASI.md` troubleshooting section
2. Verify WASI_SDK_PATH is set correctly
3. Try a clean rebuild: `rm -rf build_wasi`
4. Check wasmtime/wasmer versions
5. Review build logs for specific errors

---

**Quick Start**: `export WASI_SDK_PATH=/path/to/wasi-sdk && ./build_wasi_simple.sh`
