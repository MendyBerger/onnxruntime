#!/bin/bash
# Simple build script for ONNXRuntime with WASI-SDK using toolchain file
# Usage: ./build_wasi_simple.sh [additional cmake options]

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="${SCRIPT_DIR}/build_wasi"

export WASI_SDK_PATH="${WASI_SDK_PATH:-/opt/wasi-sdk}"

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
    -Donnxruntime_EXTENDED_MINIMAL_BUILD=ON \
    -Donnxruntime_DISABLE_EXCEPTIONS=ON \
    -Donnxruntime_DISABLE_RTTI=ON \
    -Donnxruntime_DISABLE_ABSEIL=ON \
    -Donnxruntime_USE_WEBGPU=ON \
    -Donnxruntime_WGSL_TEMPLATE=static \
    -DONNX_DISABLE_EXCEPTIONS=ON \
    "$@"

    #  -DCMAKE_BUILD_TYPE=Debug \
    # -DCMAKE_C_FLAGS="-g" \
    # -DCMAKE_CXX_FLAGS="-g" \
    # -DCMAKE_VERBOSE_MAKEFILE=ON \


# Build with verbose output
cmake --build . --target onnxruntime_webassembly -j$(nproc) --verbose

echo ""
echo "Build complete! Output files:"
find . -name "*.wasm" -type f
