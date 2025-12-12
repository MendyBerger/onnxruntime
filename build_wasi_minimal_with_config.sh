#!/bin/bash
# Minimal build script for ONNXRuntime with WASI-SDK using operator config
# This includes ONLY the operators required by TrustMark models
# Usage: ./build_wasi_minimal_with_config.sh [additional cmake options]

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="${SCRIPT_DIR}/build_wasi_minimal_config"
OPERATORS_CONFIG="${SCRIPT_DIR}/../models/required_operators_complete.config"

# Check if WASI_SDK_PATH is set
if [ -z "$WASI_SDK_PATH" ]; then
    echo "Error: WASI_SDK_PATH environment variable is not set"
    echo "Please download WASI-SDK from https://github.com/WebAssembly/wasi-sdk/releases"
    echo "and set WASI_SDK_PATH to the installation directory"
    echo ""
    echo "Example:"
    echo "  export WASI_SDK_PATH=/path/to/wasi-sdk-22.0"
    echo "  ./build_wasi_minimal_with_config.sh"
    exit 1
fi

# Check if operators config exists
if [ ! -f "$OPERATORS_CONFIG" ]; then
    echo "Error: Operators config file not found: $OPERATORS_CONFIG"
    echo ""
    echo "Please convert your .onnx models to .ort format first:"
    echo "  cd onnxruntime-wasi/tools/python"
    echo "  PYTHONPATH=/path/to/python/site-packages python3.11 convert_onnx_models_to_ort.py \\"
    echo "    ../../models --output_dir ../../models"
    echo ""
    echo "This will generate required_operators.config"
    exit 1
fi

echo "=========================================="
echo "Building ONNXRuntime MINIMAL with WASI-SDK"
echo "Using operator config for TrustMark models"
echo "=========================================="
echo "WASI-SDK Path: $WASI_SDK_PATH"
echo "Operators Config: $OPERATORS_CONFIG"
echo "Build Directory: $BUILD_DIR"
echo ""

# Display operators to be included
echo "Operators to be included:"
cat "$OPERATORS_CONFIG"
echo ""

# Create build directory
mkdir -p "$BUILD_DIR"

# Step 1: Run reduce_op_kernels.py to generate operator registration files
echo "Step 1: Generating operator registration files..."
python3.11 "$SCRIPT_DIR/tools/ci_build/reduce_op_kernels.py" \
    "$OPERATORS_CONFIG" \
    --cmake_build_dir "$BUILD_DIR" \
    --enable_type_reduction \
    --is_extended_minimal_build_or_higher

echo "✓ Operator registration files generated"
echo ""

# Step 2: Run CMake configure
echo "Step 2: Configuring CMake..."
cd "$BUILD_DIR"

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
    -Donnxruntime_ENABLE_REDUCED_OPERATOR_TYPE_SUPPORT=ON \
    -Donnxruntime_USE_WEBGPU=ON \
    -Donnxruntime_WGSL_TEMPLATE=static \
    -Donnxruntime_DISABLE_EXCEPTIONS=ON \
    -Donnxruntime_DISABLE_RTTI=ON \
    -Donnxruntime_DISABLE_ABSEIL=ON \
    -DONNX_DISABLE_EXCEPTIONS=ON \
    "$@"

echo "✓ CMake configured"
echo ""

# Step 3: Build
echo "Step 3: Building..."
cmake --build . --target onnxruntime_webassembly -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo ""
echo "=========================================="
echo "✓ Build complete! Output files:"
find . -name "*.wasm" -type f -exec ls -lh {} \;
echo ""
echo "This build includes operators specified in:"
echo "  $OPERATORS_CONFIG"
echo "=========================================="
