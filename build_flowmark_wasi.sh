#!/bin/bash
# Build ONNX Runtime for WASI with FlowMark operators
# Based on trustmark's build_wasi_minimal_with_config.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build_wasi"

# WASI SDK path
export WASI_SDK_PATH="${WASI_SDK_PATH:-/opt/wasi-sdk}"

if [ ! -d "$WASI_SDK_PATH" ]; then
    echo "ERROR: WASI SDK not found at $WASI_SDK_PATH"
    echo "Please install WASI SDK to /opt/wasi-sdk"
    exit 1
fi

echo "=========================================="
echo "Building ONNX Runtime for WASI (FlowMark)"
echo "=========================================="
echo "WASI SDK: $WASI_SDK_PATH"
echo "Build Directory: $BUILD_DIR"
echo "=========================================="

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Build ONNX Runtime with minimal operators for FlowMark
python3 ../tools/ci_build/build.py \
    --build_dir . \
    --cmake_generator "Unix Makefiles" \
    --config Release \
    --skip_tests \
    --parallel \
    --compile_no_warning_as_error \
    --disable_rtti \
    --disable_exceptions \
    --build_wasm_static_lib \
    --target wasm32-wasip2 \
    --path_to_protoc_exe $(which protoc) \
    --use_extensions \
    --minimal_build extended \
    --include_ops_by_config ../models/required_operators.config \
    --disable_ml_ops \
    --disable_contrib_ops \
    --enable_reduced_operator_type_support

echo ""
echo "=========================================="
echo "✅ ONNX Runtime WASI build complete!"
echo "=========================================="
echo "Output: $BUILD_DIR/Release/ort-wasi.wasm"
