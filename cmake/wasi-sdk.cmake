# CMake toolchain file for WASI-SDK
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=cmake/wasi-sdk.cmake ...

# Check if WASI_SDK_PATH is set
if(NOT DEFINED ENV{WASI_SDK_PATH})
    message(FATAL_ERROR "WASI_SDK_PATH environment variable is not set. "
                        "Please download WASI-SDK from https://github.com/WebAssembly/wasi-sdk/releases "
                        "and set WASI_SDK_PATH to the installation directory.")
endif()

set(WASI_SDK_PATH $ENV{WASI_SDK_PATH})

# Verify WASI-SDK exists
if(NOT EXISTS "${WASI_SDK_PATH}/bin/clang")
    message(FATAL_ERROR "WASI-SDK not found at ${WASI_SDK_PATH}")
endif()

message(STATUS "Using WASI-SDK at: ${WASI_SDK_PATH}")

# Set the target system
set(CMAKE_SYSTEM_NAME WASI)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR wasm32)

# Set the compilers - using WASI Preview 2 (wasip2)
# Must disable exceptions for component model compatibility
set(CMAKE_C_COMPILER "${WASI_SDK_PATH}/bin/wasm32-wasip2-clang")
set(CMAKE_CXX_COMPILER "${WASI_SDK_PATH}/bin/wasm32-wasip2-clang++")
set(CMAKE_AR "${WASI_SDK_PATH}/bin/llvm-ar")
set(CMAKE_RANLIB "${WASI_SDK_PATH}/bin/llvm-ranlib")
set(CMAKE_C_COMPILER_TARGET wasm32-wasip2)
set(CMAKE_CXX_COMPILER_TARGET wasm32-wasip2)

# Set the sysroot
set(CMAKE_SYSROOT "${WASI_SDK_PATH}/share/wasi-sysroot")

# Set the find root path
set(CMAKE_FIND_ROOT_PATH "${WASI_SDK_PATH}/share/wasi-sysroot")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Set C/C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags for WASI
# Use single-threaded model which provides pthread stubs (WASI-SDK 2.7+)
# Enable signal emulation for signal handling support
# Enable mmap emulation for memory mapping support
# Enable getpid emulation for process ID support
# Disable exceptions and RTTI for WASI Preview 2 component model compatibility
# Disable unwind tables to remove exception handling overhead
# Use Abseil STDCPP waiter mode (4) - standard C++ primitives for single-threaded WASI
# Disable 64-to-32 bit conversion warnings for 32-bit WebAssembly target
set(WASI_FLAGS "-D__wasi__ -mthread-model single -D_WASI_EMULATED_SIGNAL -D_WASI_EMULATED_MMAN -D_WASI_EMULATED_GETPID -fno-exceptions -fno-rtti -fno-unwind-tables -fno-asynchronous-unwind-tables -DABSL_FORCE_WAITER_MODE=4 -Wno-shorten-64-to-32")

set(CMAKE_C_FLAGS_INIT "${WASI_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${WASI_FLAGS}")

# Also set the regular flags to ensure they're applied to all targets including dependencies
set(CMAKE_C_FLAGS "${WASI_FLAGS}" CACHE STRING "C flags" FORCE)
set(CMAKE_CXX_FLAGS "${WASI_FLAGS}" CACHE STRING "CXX flags" FORCE)

# Linker flags for WASI
# Note: -mthread-model single also affects linking
# Link with wasi-emulated-signal for signal support
# Link with wasi-emulated-mman for mmap support
# Link with wasi-emulated-getpid for process ID support
set(CMAKE_EXE_LINKER_FLAGS_INIT "-Wl,--allow-undefined -Wl,--export-all -mthread-model single -lwasi-emulated-signal -lwasi-emulated-mman -lwasi-emulated-getpid")

# Cache the compiler checks
set(CMAKE_C_COMPILER_WORKS 1 CACHE INTERNAL "")
set(CMAKE_CXX_COMPILER_WORKS 1 CACHE INTERNAL "")

# Don't build shared libraries with WASI
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)

# Add compile definitions for disabled exceptions
# These are also set by onnxruntime_DISABLE_EXCEPTIONS but we set them here
# early to ensure all dependencies get them
add_compile_definitions(ORT_NO_EXCEPTIONS)
add_compile_definitions(ONNX_NO_EXCEPTIONS)
add_compile_definitions(MLAS_NO_EXCEPTION)
add_compile_definitions(JSON_NOEXCEPTION)

message(STATUS "WASI-SDK toolchain loaded (Preview 2)")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "  Target: wasm32-wasip2")
message(STATUS "  Sysroot: ${CMAKE_SYSROOT}")
message(STATUS "  Thread Model: single (pthread stubs)")
message(STATUS "  Exceptions: DISABLED (component model requirement)")
message(STATUS "  Signal Support: emulated")
message(STATUS "  Mmap Support: emulated")
message(STATUS "  Process ID: emulated")
