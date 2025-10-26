#!/bin/bash
# Simple build script for ONNXRuntime with WASI-SDK using toolchain file
# Usage: ./build_wasi_simple.sh [additional cmake options]

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="${SCRIPT_DIR}/build_wasi"

# Check if WASI_SDK_PATH is set
if [ -z "$WASI_SDK_PATH" ]; then
    echo "Error: WASI_SDK_PATH environment variable is not set"
    echo "Please download WASI-SDK from https://github.com/WebAssembly/wasi-sdk/releases"
    echo "and set WASI_SDK_PATH to the installation directory"
    echo ""
    echo "Example:"
    echo "  export WASI_SDK_PATH=/path/to/wasi-sdk-22.0"
    echo "  ./build_wasi_simple.sh"
    exit 1
fi

echo "Building ONNXRuntime with WASI-SDK"
echo "WASI-SDK Path: $WASI_SDK_PATH"

# Create and enter build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake using toolchain file
# Note: ONNXRuntime's CMakeLists.txt is in the cmake/ subdirectory
# Using WASI-SDK 2.7+ with -mthread-model single for pthread stubs
# WASI Preview 2 requires exceptions to be disabled
cmake "$SCRIPT_DIR/cmake" \
    -DCMAKE_TOOLCHAIN_FILE="$SCRIPT_DIR/cmake/wasi-sdk.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -Donnxruntime_BUILD_SHARED_LIB=OFF \
    -Donnxruntime_BUILD_WEBASSEMBLY_STATIC_LIB=OFF \
    -Donnxruntime_ENABLE_WEBASSEMBLY_SIMD=ON \
    -Donnxruntime_USE_XNNPACK=OFF \
    -Donnxruntime_BUILD_UNIT_TESTS=OFF \
    -Donnxruntime_BUILD_BENCHMARKS=OFF \
    -Donnxruntime_MINIMAL_BUILD=ON \
    -Donnxruntime_DISABLE_EXCEPTIONS=ON \
    -Donnxruntime_DISABLE_RTTI=ON \
    -DONNX_DISABLE_EXCEPTIONS=ON \
    "$@"

# Build
cmake --build . --target onnxruntime_webassembly -j$(nproc)

echo ""
echo "Build complete! Output files:"
find . -name "*.wasm" -type f
