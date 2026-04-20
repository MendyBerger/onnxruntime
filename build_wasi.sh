#!/bin/bash
# Build ONNXRuntime with WASI-SDK
# Usage: ./build_wasi.sh [debug|release] [options]

set -e

# Configuration
BUILD_TYPE="${1:-Release}"
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="${SCRIPT_DIR}/build_wasi"

# WASI-SDK path - can be overridden with environment variable
if [ -z "$WASI_SDK_PATH" ]; then
    echo "Error: WASI_SDK_PATH environment variable is not set"
    echo "Please download WASI-SDK from https://github.com/WebAssembly/wasi-sdk/releases"
    echo "and set WASI_SDK_PATH to the installation directory"
    exit 1
fi

# Verify WASI-SDK installation
if [ ! -f "$WASI_SDK_PATH/bin/clang" ]; then
    echo "Error: WASI-SDK not found at $WASI_SDK_PATH"
    exit 1
fi

echo "=========================================="
echo "Building ONNXRuntime with WASI-SDK"
echo "=========================================="
echo "WASI-SDK Path: $WASI_SDK_PATH"
echo "Build Type: $BUILD_TYPE"
echo "Build Directory: $BUILD_DIR"
echo "=========================================="

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# CMake configuration
# Note: ONNXRuntime's CMakeLists.txt is in the cmake/ subdirectory
cmake "$SCRIPT_DIR/cmake" \
    -DCMAKE_SYSTEM_NAME=WASI \
    -DCMAKE_SYSTEM_VERSION=1 \
    -DCMAKE_SYSTEM_PROCESSOR=wasm32 \
    -DCMAKE_C_COMPILER="$WASI_SDK_PATH/bin/clang" \
    -DCMAKE_CXX_COMPILER="$WASI_SDK_PATH/bin/clang++" \
    -DCMAKE_AR="$WASI_SDK_PATH/bin/llvm-ar" \
    -DCMAKE_RANLIB="$WASI_SDK_PATH/bin/llvm-ranlib" \
    -DCMAKE_C_COMPILER_TARGET=wasm32-wasi \
    -DCMAKE_CXX_COMPILER_TARGET=wasm32-wasi \
    -DCMAKE_C_FLAGS="-Wno-deprecated -D_WASI_EMULATED_SIGNAL -D_WASI_EMULATED_MMAN" \
    -DCMAKE_CXX_FLAGS="-Wno-deprecated -D_WASI_EMULATED_SIGNAL -D_WASI_EMULATED_MMAN" \
    -DCMAKE_EXE_LINKER_FLAGS="-lwasi-emulated-signal -lwasi-emulated-mman" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_SYSROOT="$WASI_SDK_PATH/share/wasi-sysroot" \
    -DCMAKE_FIND_ROOT_PATH="$WASI_SDK_PATH/share/wasi-sysroot" \
    -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
    -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
    -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
    -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY \
    -Donnxruntime_BUILD_SHARED_LIB=OFF \
    -Donnxruntime_BUILD_WEBASSEMBLY_STATIC_LIB=OFF \
    -Donnxruntime_ENABLE_WEBASSEMBLY_SIMD=ON \
    -Donnxruntime_ENABLE_WEBASSEMBLY_THREADS=OFF \
    -Donnxruntime_USE_XNNPACK=OFF \
    -Donnxruntime_USE_JSEP=OFF \
    -Donnxruntime_USE_WEBGPU=OFF \
    -Donnxruntime_USE_WEBNN=OFF \
    -Donnxruntime_BUILD_UNIT_TESTS=OFF \
    -Donnxruntime_BUILD_BENCHMARKS=OFF \
    -Donnxruntime_MINIMAL_BUILD=ON \
    -Donnxruntime_DISABLE_EXCEPTIONS=ON \
    -Donnxruntime_DISABLE_RTTI=ON \
    -DONNX_DISABLE_EXCEPTIONS=ON \
    -Donnxruntime_EXTENDED_MINIMAL_BUILD=OFF \
    -Donnxruntime_DISABLE_CONTRIB_OPS=OFF \
    -Donnxruntime_DISABLE_ML_OPS=OFF \
    -DCMAKE_VERBOSE_MAKEFILE=ON \
    "$@"

# Build
echo ""
echo "=========================================="
echo "Starting build..."
echo "=========================================="
cmake --build . --target onnxruntime_webassembly -j$(nproc)

# Convert WASM module to WASI Preview 2 component
# The linker produces a plain WASM module; wasm-tools wraps it into a component
# with the WASI P1 adapter so graphtime/wasmtime can run it.
WASM_OUTPUT="$(find . -name 'ort-wasi*.wasm' -type f | head -1)"
if [ -n "$WASM_OUTPUT" ] && command -v wasm-tools >/dev/null 2>&1; then
    echo ""
    echo "=========================================="
    echo "Converting to WASI Preview 2 component..."
    echo "=========================================="
    ADAPTER=$(find ~/.cargo/registry -name "wasi_snapshot_preview1.command.wasm" \
        -path "*/wasi-preview1-component-adapter-provider*" 2>/dev/null | sort -rV | head -1)
    if [ -z "$ADAPTER" ]; then
        echo "Warning: wasi_snapshot_preview1.command.wasm adapter not found in ~/.cargo/registry"
        echo "Install it with: cargo add wasi-preview1-component-adapter-provider"
        echo "Skipping component conversion — binary will not run with graphtime"
    else
        wasm-tools component new "$WASM_OUTPUT" \
            --adapt "wasi_snapshot_preview1=$ADAPTER" \
            -o "$WASM_OUTPUT"
        echo "Component created: $WASM_OUTPUT"
        # Verify component magic
        MAGIC=$(xxd "$WASM_OUTPUT" | head -1 | awk '{print $3}')
        if [ "$MAGIC" = "0d00" ]; then
            echo "✓ Verified: WASI Preview 2 component (magic bytes ok)"
        else
            echo "Warning: unexpected magic bytes — may not be a valid component"
        fi
    fi
fi

# Show output
echo ""
echo "=========================================="
echo "Build complete!"
echo "=========================================="
echo "Output files:"
find . -name "*.wasm" -type f

echo ""
echo "To run with graphtime (WebGPU):"
echo "  USE_WEBGPU=1 graphtime --dir=. $(find . -name 'ort-wasi*.wasm' -type f | head -1)"
echo ""
echo "To run with wasmtime (CPU only):"
echo "  wasmtime --dir=. $(find . -name 'ort-wasi*.wasm' -type f | head -1)"
