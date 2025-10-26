# Building ONNXRuntime with WASI-SDK

This guide explains how to build ONNXRuntime for WebAssembly using WASI-SDK instead of Emscripten.

## Table of Contents

- [Overview](#overview)
- [Prerequisites](#prerequisites)
- [Quick Start](#quick-start)
- [Build Options](#build-options)
- [Running WASI Binaries](#running-wasi-binaries)
- [Integration Examples](#integration-examples)
- [Troubleshooting](#troubleshooting)

## Overview

WASI (WebAssembly System Interface) is a standardized system interface for WebAssembly. Building ONNXRuntime with WASI-SDK offers several advantages over Emscripten:

- **Standardization**: WASI is a W3C standard
- **Portability**: Runs on multiple WASI runtimes (wasmtime, wasmer, etc.)
- **Simplicity**: No JavaScript glue code required
- **Server-Side**: Better suited for server-side WebAssembly workloads
- **Performance**: Direct system interface without browser overhead

## Prerequisites

### 1. Install WASI-SDK

Download the latest WASI-SDK from the [official releases](https://github.com/WebAssembly/wasi-sdk/releases):

```bash
# Download WASI-SDK (adjust version as needed)
wget https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-22/wasi-sdk-22.0-linux.tar.gz
tar xzf wasi-sdk-22.0-linux.tar.gz
export WASI_SDK_PATH="$(pwd)/wasi-sdk-22.0"
```

### 2. Install CMake

CMake 3.18 or higher is required:

```bash
cmake --version  # Check if installed
```

### 3. Install a WASI Runtime (for testing)

**Option A: Wasmtime**
```bash
curl https://wasmtime.dev/install.sh -sSf | bash
```

**Option B: Wasmer**
```bash
curl https://get.wasmer.io -sSf | sh
```

## Quick Start

### Method 1: Using the Simple Build Script

```bash
# Set WASI-SDK path
export WASI_SDK_PATH=/path/to/wasi-sdk-22.0

# Build ONNXRuntime
./build_wasi_simple.sh
```

The output will be in `build_wasi/` directory with a filename like `ort-wasi-simd.wasm`.

### Method 2: Using the Detailed Build Script

```bash
# Set WASI-SDK path
export WASI_SDK_PATH=/path/to/wasi-sdk-22.0

# Build in release mode (default)
./build_wasi.sh

# Or build in debug mode
./build_wasi.sh debug
```

### Method 3: Manual CMake Configuration

```bash
export WASI_SDK_PATH=/path/to/wasi-sdk-22.0

mkdir build_wasi && cd build_wasi

# Note: ONNXRuntime's CMakeLists.txt is in the cmake/ subdirectory
cmake ../cmake \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/wasi-sdk.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -Donnxruntime_BUILD_WEBASSEMBLY_STATIC_LIB=OFF \
    -Donnxruntime_ENABLE_WEBASSEMBLY_SIMD=ON \
    -Donnxruntime_USE_XNNPACK=OFF

cmake --build . --target onnxruntime_webassembly -j$(nproc)
```

**Note**: XNNPACK is currently disabled by default for WASI builds as it doesn't yet recognize WASI as a platform. Basic SIMD optimizations are still enabled.

## Build Options

### Common CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `onnxruntime_ENABLE_WEBASSEMBLY_SIMD` | ON | Enable WebAssembly SIMD instructions |
| `onnxruntime_ENABLE_WEBASSEMBLY_RELAXED_SIMD` | OFF | Enable relaxed SIMD (faster, less precise) |
| `onnxruntime_USE_XNNPACK` | ON | Use XNNPACK for CPU optimization |
| `onnxruntime_ENABLE_TRAINING_APIS` | OFF | Include training APIs |
| `onnxruntime_BUILD_WEBASSEMBLY_STATIC_LIB` | OFF | Build as static library |
| `onnxruntime_BUILD_UNIT_TESTS` | OFF | Build unit tests |
| `onnxruntime_DISABLE_RTTI` | OFF | Disable RTTI |
| `CMAKE_BUILD_TYPE` | Release | Debug or Release |

### Build Variants

```bash
# Standard build with SIMD
./build_wasi_simple.sh

# Build with relaxed SIMD
./build_wasi_simple.sh -Donnxruntime_ENABLE_WEBASSEMBLY_RELAXED_SIMD=ON

# Build with training APIs
./build_wasi_simple.sh -Donnxruntime_ENABLE_TRAINING_APIS=ON

# Debug build
./build_wasi_simple.sh -DCMAKE_BUILD_TYPE=Debug

# Minimal build
./build_wasi_simple.sh \
    -Donnxruntime_MINIMAL_BUILD=ON \
    -Donnxruntime_DISABLE_CONTRIB_OPS=ON
```

## Running WASI Binaries

### Using Wasmtime

```bash
# Run the WebAssembly module
wasmtime run --dir=. ort-wasi-simd.wasm

# Run with additional WASI capabilities
wasmtime run \
    --dir=. \
    --dir=/tmp:/tmp \
    --env KEY=VALUE \
    ort-wasi-simd.wasm
```

### Using Wasmer

```bash
# Run the WebAssembly module
wasmer run ort-wasi-simd.wasm

# Run with directory mapping
wasmer run --dir=. ort-wasi-simd.wasm
```

### In Node.js

Using `@wasmer/wasi`:

```javascript
const { init, WASI } = require('@wasmer/wasi');
const fs = require('fs');

(async () => {
    await init();

    const wasi = new WASI({
        args: [],
        env: {},
    });

    const wasmBinary = fs.readFileSync('./ort-wasi-simd.wasm');
    const module = await WebAssembly.compile(wasmBinary);
    const instance = await WebAssembly.instantiate(module, {
        ...wasi.getImports(module)
    });

    wasi.start(instance);
})();
```

### In Browser

Using `@wasmer/wasi` for browser:

```javascript
import { init, WASI } from '@wasmer/wasi';

async function loadONNXRuntime() {
    await init();

    const response = await fetch('ort-wasi-simd.wasm');
    const wasmBinary = await response.arrayBuffer();

    const wasi = new WASI({
        args: [],
        env: {},
    });

    const module = await WebAssembly.compile(wasmBinary);
    const instance = await WebAssembly.instantiate(module, {
        ...wasi.getImports(module)
    });

    wasi.start(instance);

    return instance.exports;
}
```

## Integration Examples

### Python with wasmtime

```python
from wasmtime import Store, Module, Instance

# Load the WASM module
store = Store()
module = Module.from_file(store.engine, 'ort-wasi-simd.wasm')
instance = Instance(store, module, [])

# Access exports
exports = instance.exports(store)
```

### Rust with wasmtime

```rust
use wasmtime::*;

fn main() -> Result<()> {
    let engine = Engine::default();
    let module = Module::from_file(&engine, "ort-wasi-simd.wasm")?;

    let mut store = Store::new(&engine, ());
    let instance = Instance::new(&mut store, &module, &[])?;

    // Access exports
    // let func = instance.get_func(&mut store, "function_name")?;

    Ok(())
}
```

## Troubleshooting

### Build Issues

**Problem**: `WASI_SDK_PATH environment variable is not set`
```bash
# Solution: Set the environment variable
export WASI_SDK_PATH=/path/to/wasi-sdk-22.0
```

**Problem**: `clang: error: unsupported option '-pthread'`
```bash
# Solution: Ensure you're using the WASI-SDK toolchain file
cmake -DCMAKE_TOOLCHAIN_FILE=cmake/wasi-sdk.cmake ...
```

**Problem**: Linker errors about missing symbols
```bash
# Solution: Add --allow-undefined flag (already in onnxruntime_webassembly.cmake)
# Or provide the missing symbols through imports
```

### Runtime Issues

**Problem**: `unknown import: wasi_snapshot_preview1::...`
```bash
# Solution: Use a WASI runtime that supports wasi_snapshot_preview1
wasmtime run --wasi-modules=wasi_snapshot_preview1 module.wasm
```

**Problem**: Module crashes or segfaults
```bash
# Solution: Build in debug mode and check stack traces
./build_wasi_simple.sh -DCMAKE_BUILD_TYPE=Debug
wasmtime run --invoke-func-name _start module.wasm
```

### Performance Issues

**Problem**: Slow inference
```bash
# Solutions:
# 1. Enable SIMD
./build_wasi_simple.sh -Donnxruntime_ENABLE_WEBASSEMBLY_SIMD=ON

# 2. Use relaxed SIMD for better performance (slightly less precise)
./build_wasi_simple.sh -Donnxruntime_ENABLE_WEBASSEMBLY_RELAXED_SIMD=ON

# 3. Enable XNNPACK
./build_wasi_simple.sh -Donnxruntime_USE_XNNPACK=ON

# 4. Use release build with optimizations
./build_wasi_simple.sh -DCMAKE_BUILD_TYPE=Release
```

## Features Not Supported in WASI

The following features are specific to Emscripten and browser environments:

- **JSEP** (JavaScript Execution Provider) - requires browser JavaScript APIs
- **WebGPU** - requires browser GPU APIs
- **WebNN** - requires browser Web Neural Network API
- **Traditional Threading** - WASI uses a different threading model
- **Asyncify/JSPI** - Emscripten-specific async handling
- **Iconv** - Character encoding conversion (WASI libc doesn't include it, not needed for inference)
- **XNNPACK** - CPU optimization library (doesn't recognize WASI as a platform yet)

These features will be automatically disabled when building with WASI-SDK. Basic SIMD optimizations remain available.

## Advantages of WASI Over Emscripten

1. **Smaller Binary Size**: No JavaScript glue code
2. **Faster Startup**: Direct instantiation without JS initialization
3. **Better Portability**: Runs on any WASI-compliant runtime
4. **Server-Side Focus**: Better suited for backend workloads
5. **Standardized Interface**: Uses W3C WASI standard

## When to Use WASI vs Emscripten

**Use WASI when:**
- Building for server-side WebAssembly
- Running in cloud functions or edge computing
- Need maximum portability across runtimes
- Don't need browser-specific features
- Want smaller binary size

**Use Emscripten when:**
- Building for web browsers
- Need JSEP, WebGPU, or WebNN support
- Require extensive JavaScript interop
- Need traditional pthread threading

## Resources

- [WASI Official Site](https://wasi.dev/)
- [WASI-SDK Repository](https://github.com/WebAssembly/wasi-sdk)
- [Wasmtime Documentation](https://docs.wasmtime.dev/)
- [Wasmer Documentation](https://docs.wasmer.io/)
- [WASI Specification](https://github.com/WebAssembly/WASI)
- [ONNXRuntime Documentation](https://onnxruntime.ai/)

## Contributing

If you encounter issues or have improvements for the WASI build:

1. Check existing issues in the ONNXRuntime repository
2. Report bugs with detailed build/runtime logs
3. Submit pull requests with improvements
4. Update documentation as needed

## License

ONNXRuntime is licensed under the MIT License. See the LICENSE file for details.
