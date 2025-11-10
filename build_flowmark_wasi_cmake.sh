#!/bin/bash
# Build ONNX Runtime for WASI - FlowMark version
# Based on trustmark's build_wasi_minimal_with_config.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build_wasi"
OPERATORS_CONFIG="${SCRIPT_DIR}/models/required_operators.config"

# Check if WASI_SDK_PATH is set
if [ -z "$WASI_SDK_PATH" ]; then
    echo "Error: WASI_SDK_PATH environment variable is not set"
    echo "Please set it to /opt/wasi-sdk"
    exit 1
fi

echo "=========================================="
echo "Building ONNX Runtime for WASI (FlowMark)"
echo "=========================================="
echo "WASI-SDK Path: $WASI_SDK_PATH"
echo "Operators Config: $OPERATORS_CONFIG"
echo "Build Directory: $BUILD_DIR"
echo ""

# Create build directory (clean op_reduction.generated to ensure fresh operator registration)
mkdir -p "$BUILD_DIR"
if [ -d "$BUILD_DIR/op_reduction.generated" ]; then
    echo "Cleaning previous operator registration files..."
    rm -rf "$BUILD_DIR/op_reduction.generated"
fi

# Step 1: Run reduce_op_kernels.py
echo "Step 1: Generating operator registration files..."
echo "Using config file: $OPERATORS_CONFIG"
python3.11 "$SCRIPT_DIR/tools/ci_build/reduce_op_kernels.py" \
    "$OPERATORS_CONFIG" \
    --cmake_build_dir "$BUILD_DIR" \
    --enable_type_reduction \
    --use_webgpu \
    --is_extended_minimal_build_or_higher

echo "✓ Operator registration files generated"

# Verify NHWC Conv operator was included
if [ -d "$BUILD_DIR/op_reduction.generated" ]; then
    echo ""
    echo "Verifying NHWC Conv operator inclusion..."
    if grep -r "com.ms.internal.nhwc.*Conv\|kMSInternalNHWCDomain.*Conv" "$BUILD_DIR/op_reduction.generated" > /dev/null 2>&1; then
        echo "✓ NHWC Conv operator found in generated files"
    else
        echo "⚠️  WARNING: NHWC Conv operator not found in generated files"
        echo "   Checking WebGPU provider files..."
        if [ -f "$BUILD_DIR/op_reduction.generated/onnxruntime/core/providers/webgpu/webgpu_execution_provider.cc" ]; then
            if grep -q "kMSInternalNHWCDomain.*Conv" "$BUILD_DIR/op_reduction.generated/onnxruntime/core/providers/webgpu/webgpu_execution_provider.cc" 2>/dev/null; then
                echo "✓ NHWC Conv operator found in WebGPU provider file"
            else
                echo "⚠️  WARNING: NHWC Conv operator not found in WebGPU provider file"
            fi
        else
            echo "⚠️  WARNING: WebGPU provider file not found in generated files"
        fi
    fi
fi
echo ""

# Step 2: Run CMake configure
echo "Step 2: Configuring CMake..."
cd "$BUILD_DIR"

# Clean CMake cache to ensure fresh configuration
if [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
    echo "Cleaning CMake cache..."
    rm -f "$BUILD_DIR/CMakeCache.txt"
    rm -rf "$BUILD_DIR/CMakeFiles"
fi

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
    -Donnxruntime_DISABLE_EXCEPTIONS=ON \
    -Donnxruntime_DISABLE_RTTI=ON \
    -Donnxruntime_DISABLE_ABSEIL=ON \
    -Donnxruntime_USE_WEBGPU=ON \
    -Donnxruntime_USE_EXTERNAL_DAWN=OFF \
    -Donnxruntime_WGSL_TEMPLATE=static \
    -DONNX_DISABLE_EXCEPTIONS=ON

echo "✓ CMake configured"
echo ""

# Step 3: Build
echo "Step 3: Building..."
cmake --build . --target onnxruntime_webassembly -j$(sysctl -n hw.ncpu)

echo ""
echo "=========================================="
echo "✓ Build complete! Output files:"
find . -name "*.wasm" -type f -exec ls -lh {} \;
echo "=========================================="
