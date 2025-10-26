#!/bin/bash
# Verification script for WASI-SDK build setup
# Usage: ./verify_wasi_setup.sh

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo "=========================================="
echo "WASI-SDK Setup Verification"
echo "=========================================="
echo ""

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to check if command exists
check_command() {
    if command -v "$1" &> /dev/null; then
        echo -e "${GREEN}✓${NC} $1 is installed"
        if [ "$2" = "version" ]; then
            echo "  Version: $("$1" --version | head -1)"
        fi
        return 0
    else
        echo -e "${RED}✗${NC} $1 is not installed"
        return 1
    fi
}

# Function to check file exists
check_file() {
    if [ -f "$1" ]; then
        echo -e "${GREEN}✓${NC} $2"
        return 0
    else
        echo -e "${RED}✗${NC} $2 (not found: $1)"
        return 1
    fi
}

# Function to check directory exists
check_dir() {
    if [ -d "$1" ]; then
        echo -e "${GREEN}✓${NC} $2"
        return 0
    else
        echo -e "${RED}✗${NC} $2 (not found: $1)"
        return 1
    fi
}

ERRORS=0

# Check 1: WASI_SDK_PATH environment variable
echo "Checking WASI-SDK installation..."
echo "---"
if [ -z "$WASI_SDK_PATH" ]; then
    echo -e "${RED}✗${NC} WASI_SDK_PATH environment variable is not set"
    echo -e "  ${YELLOW}Please set it to your WASI-SDK installation directory${NC}"
    echo -e "  Example: export WASI_SDK_PATH=/path/to/wasi-sdk-22.0"
    ERRORS=$((ERRORS + 1))
else
    echo -e "${GREEN}✓${NC} WASI_SDK_PATH is set: $WASI_SDK_PATH"

    # Check if WASI-SDK directory exists
    if [ ! -d "$WASI_SDK_PATH" ]; then
        echo -e "${RED}✗${NC} WASI_SDK_PATH directory does not exist"
        ERRORS=$((ERRORS + 1))
    else
        echo -e "${GREEN}✓${NC} WASI_SDK_PATH directory exists"

        # Check for required WASI-SDK components
        check_file "$WASI_SDK_PATH/bin/clang" "WASI clang compiler" || ERRORS=$((ERRORS + 1))
        check_file "$WASI_SDK_PATH/bin/clang++" "WASI clang++ compiler" || ERRORS=$((ERRORS + 1))
        check_file "$WASI_SDK_PATH/bin/llvm-ar" "WASI llvm-ar" || ERRORS=$((ERRORS + 1))
        check_file "$WASI_SDK_PATH/bin/llvm-ranlib" "WASI llvm-ranlib" || ERRORS=$((ERRORS + 1))
        check_dir "$WASI_SDK_PATH/share/wasi-sysroot" "WASI sysroot" || ERRORS=$((ERRORS + 1))
    fi
fi
echo ""

# Check 2: CMake
echo "Checking build tools..."
echo "---"
check_command cmake version || ERRORS=$((ERRORS + 1))

if command -v cmake &> /dev/null; then
    CMAKE_VERSION=$(cmake --version | head -1 | grep -oP '\d+\.\d+' | head -1)
    CMAKE_MAJOR=$(echo $CMAKE_VERSION | cut -d. -f1)
    CMAKE_MINOR=$(echo $CMAKE_VERSION | cut -d. -f2)

    if [ "$CMAKE_MAJOR" -lt 3 ] || ([ "$CMAKE_MAJOR" -eq 3 ] && [ "$CMAKE_MINOR" -lt 18 ]); then
        echo -e "${YELLOW}⚠${NC}  CMake version $CMAKE_VERSION detected. Version 3.18+ recommended."
    fi
fi

check_command make || ERRORS=$((ERRORS + 1))
check_command python3 || ERRORS=$((ERRORS + 1))
echo ""

# Check 3: WASI Runtimes (optional but recommended)
echo "Checking WASI runtimes (optional)..."
echo "---"
RUNTIME_FOUND=0

if check_command wasmtime version; then
    RUNTIME_FOUND=1
fi

if check_command wasmer version; then
    RUNTIME_FOUND=1
fi

if [ $RUNTIME_FOUND -eq 0 ]; then
    echo -e "${YELLOW}⚠${NC}  No WASI runtime found. Install wasmtime or wasmer to run .wasm files."
    echo -e "  Wasmtime: curl https://wasmtime.dev/install.sh -sSf | bash"
    echo -e "  Wasmer:   curl https://get.wasmer.io -sSf | sh"
fi
echo ""

# Check 4: Build scripts
echo "Checking build scripts..."
echo "---"
check_file "$SCRIPT_DIR/build_wasi_simple.sh" "Simple build script" || ERRORS=$((ERRORS + 1))
check_file "$SCRIPT_DIR/build_wasi.sh" "Detailed build script" || ERRORS=$((ERRORS + 1))
check_file "$SCRIPT_DIR/cmake/wasi-sdk.cmake" "WASI toolchain file" || ERRORS=$((ERRORS + 1))
check_file "$SCRIPT_DIR/cmake/onnxruntime_webassembly.cmake" "WebAssembly build config" || ERRORS=$((ERRORS + 1))
echo ""

# Check 5: Documentation
echo "Checking documentation..."
echo "---"
check_file "$SCRIPT_DIR/README_WASI.md" "WASI README" || ERRORS=$((ERRORS + 1))
check_file "$SCRIPT_DIR/WASI_QUICK_REFERENCE.md" "Quick reference" || ERRORS=$((ERRORS + 1))
check_file "$SCRIPT_DIR/WASI_SDK_MIGRATION.md" "Migration guide" || ERRORS=$((ERRORS + 1))
echo ""

# Summary
echo "=========================================="
echo "Verification Summary"
echo "=========================================="
echo ""

if [ $ERRORS -eq 0 ]; then
    echo -e "${GREEN}✓ All checks passed!${NC}"
    echo ""
    echo "Your system is ready to build ONNXRuntime with WASI-SDK."
    echo ""
    echo "Next steps:"
    echo "  1. Run: ./build_wasi_simple.sh"
    echo "  2. Wait for build to complete"
    echo "  3. Find output in build_wasi/ directory"

    if [ $RUNTIME_FOUND -eq 1 ]; then
        echo "  4. Test with: wasmtime run --dir=. build_wasi/ort-wasi*.wasm"
    fi
    echo ""
    echo "For more information, see README_WASI.md"
    exit 0
else
    echo -e "${RED}✗ $ERRORS error(s) found${NC}"
    echo ""
    echo "Please fix the errors above before building."
    echo ""
    echo "Common fixes:"
    echo "  • Set WASI_SDK_PATH: export WASI_SDK_PATH=/path/to/wasi-sdk"
    echo "  • Install CMake 3.18+: https://cmake.org/download/"
    echo "  • Install build tools: sudo apt-get install build-essential"
    echo ""
    echo "For detailed setup instructions, see README_WASI.md"
    exit 1
fi
