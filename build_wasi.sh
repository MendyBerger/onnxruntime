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
    -Donnxruntime_MINIMAL_BUILD=OFF \
    -Donnxruntime_EXTENDED_MINIMAL_BUILD=OFF \
    -Donnxruntime_DISABLE_CONTRIB_OPS=OFF \
    -Donnxruntime_DISABLE_ML_OPS=OFF \
    -Donnxruntime_DISABLE_RTTI=OFF \
    -Donnxruntime_DISABLE_EXCEPTIONS=OFF \
    -Donnxruntime_BUILD_BENCHMARKS=OFF \
    -DCMAKE_VERBOSE_MAKEFILE=ON \
    "$@"

# Build
echo ""
echo "=========================================="
echo "Starting build..."
echo "=========================================="
cmake --build . --target onnxruntime_webassembly -j$(nproc)

# Show output
echo ""
echo "=========================================="
echo "Build complete!"
echo "=========================================="
echo "Output files:"
find . -name "*.wasm" -type f

echo ""
echo "To run with wasmtime:"
echo "  wasmtime run --dir=. $(find . -name 'ort-wasi*.wasm' -type f | head -1)"
echo ""
echo "To run with wasmer:"
echo "  wasmer run $(find . -name 'ort-wasi*.wasm' -type f | head -1)"
