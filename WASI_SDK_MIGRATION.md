# WASI-SDK Migration for ONNXRuntime

This document describes the changes made to adapt ONNXRuntime's WebAssembly build system from Emscripten to WASI-SDK.

## Overview

The `cmake/onnxruntime_webassembly.cmake` file has been adapted to use WASI-SDK instead of Emscripten. WASI (WebAssembly System Interface) provides a more standardized approach to WebAssembly builds with better portability.

## Key Changes

### 1. Removed Emscripten-Specific Features

- **JavaScript Interop**: Removed all Emscripten-specific JavaScript injection (pre.js, post.js files)
- **Emscripten Flags**: Removed all `-s` flags (e.g., `EXPORTED_RUNTIME_METHODS`, `MODULARIZE`, etc.)
- **JSEP/WebGPU**: Disabled JSEP and WebGPU support as they require browser-specific features
- **Threading**: Disabled traditional pthread-based threading (not supported in standard WASI)
- **Asyncify/JSPI**: Removed Emscripten-specific async handling

### 2. Added WASI-SDK Configuration

#### Compiler Definitions
```cmake
add_compile_definitions(
  BUILD_MLAS_NO_ONNXRUNTIME
  __wasi__
)
```

#### Linker Options
- `--allow-undefined`: Allow undefined symbols (to be provided by the WASI runtime)
- `--export-all`: Export all symbols from the WebAssembly module
- `--no-entry`: No main entry point required
- `--stack-first`: Stack placement optimization
- `-z,stack-size=1048576`: 1MB stack size

#### Memory Configuration
- **Debug Build**: 64MB initial, 2GB max
- **Release Build**: 16MB initial, 4GB max

### 3. SIMD Support

WASI-SDK supports WebAssembly SIMD instructions:
- `-msimd128`: Standard SIMD support
- `-mrelaxed-simd`: Relaxed SIMD instructions (if enabled)

### 4. Output Format

- Changed output extension from `.mjs` to `.wasm`
- Output naming: `ort-wasi[-simd][-relaxedsimd][-training].wasm`

### 5. Testing

Tests now run using a WASI runtime (e.g., wasmtime):
```cmake
add_test(NAME onnxruntime_webassembly_test
  COMMAND wasmtime run --dir=. $<TARGET_FILE:onnxruntime_webassembly_test>
  WORKING_DIRECTORY $<TARGET_FILE_DIR:onnxruntime_webassembly_test>
)
```

## Building with WASI-SDK

### Prerequisites

1. Install WASI-SDK from: https://github.com/WebAssembly/wasi-sdk/releases
2. Set up the environment:
   ```bash
   export WASI_SDK_PATH=/path/to/wasi-sdk
   export CC="$WASI_SDK_PATH/bin/clang"
   export CXX="$WASI_SDK_PATH/bin/clang++"
   export AR="$WASI_SDK_PATH/bin/llvm-ar"
   export RANLIB="$WASI_SDK_PATH/bin/llvm-ranlib"
   ```

### Build Commands

```bash
mkdir build && cd build

# Note: ONNXRuntime's CMakeLists.txt is in the cmake/ subdirectory
cmake ../cmake \
  -DCMAKE_SYSTEM_NAME=WASI \
  -DCMAKE_SYSTEM_VERSION=1 \
  -DCMAKE_SYSTEM_PROCESSOR=wasm32 \
  -DCMAKE_C_COMPILER="$WASI_SDK_PATH/bin/clang" \
  -DCMAKE_CXX_COMPILER="$WASI_SDK_PATH/bin/clang++" \
  -DCMAKE_AR="$WASI_SDK_PATH/bin/llvm-ar" \
  -DCMAKE_RANLIB="$WASI_SDK_PATH/bin/llvm-ranlib" \
  -DCMAKE_C_COMPILER_TARGET=wasm32-wasi \
  -DCMAKE_CXX_COMPILER_TARGET=wasm32-wasi \
  -DCMAKE_BUILD_TYPE=Release \
  -Donnxruntime_BUILD_WEBASSEMBLY_STATIC_LIB=OFF \
  -Donnxruntime_ENABLE_WEBASSEMBLY_SIMD=ON \
  -Donnxruntime_USE_XNNPACK=OFF

cmake --build . --target onnxruntime_webassembly
```

**Note**: XNNPACK is disabled as it doesn't currently recognize WASI as a valid platform. SIMD optimizations are still enabled.

## Running WASI Binaries

WASI binaries require a WASI runtime to execute:

### Using wasmtime
```bash
wasmtime run --dir=. ort-wasi.wasm
```

### Using wasmer
```bash
wasmer run ort-wasi.wasm
```

### In Browser (with polyfill)
WASI binaries can run in browsers using JavaScript polyfills like:
- [@wasmer/wasi](https://github.com/wasmerio/wasmer-js)
- [browser_wasi_shim](https://github.com/bjorn3/browser_wasi_shim)

## Limitations

### Not Supported with WASI-SDK
1. **JSEP**: JavaScript Execution Provider requires Emscripten's JS interop
2. **WebGPU**: Direct GPU access requires browser APIs
3. **WebNN**: Web Neural Network API requires browser integration
4. **Traditional Threading**: pthread-based threading (WASI threads are different)

### Recommendations
- Use XNNPACK for CPU optimization
- Consider WASI threads (when stable) for parallelism
- For browser-specific features, consider keeping a separate Emscripten build

## Benefits of WASI-SDK

1. **Standardization**: WASI is a standard interface for WebAssembly
2. **Portability**: Runs on multiple WASI runtimes (wasmtime, wasmer, etc.)
3. **Simplicity**: No JavaScript glue code required
4. **Performance**: Direct system interface without browser overhead
5. **Server-Side**: Better support for server-side WebAssembly workloads

## Migration Notes

- The static library bundling function remains unchanged
- Core ONNXRuntime functionality is preserved
- Training APIs are still supported
- SIMD optimizations work with WASI-SDK

## References

- [WASI SDK](https://github.com/WebAssembly/wasi-sdk)
- [WASI Specification](https://github.com/WebAssembly/WASI)
- [Wasmtime Runtime](https://github.com/bytecodealliance/wasmtime)
- [Wasmer Runtime](https://github.com/wasmerio/wasmer)
