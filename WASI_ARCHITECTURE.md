# ONNXRuntime WASI Architecture

This document visualizes the WASI-SDK build architecture for ONNXRuntime.

## Build System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         Build Entry Points                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌──────────────────┐  ┌──────────────────┐  ┌───────────────┐ │
│  │ build_wasi.sh    │  │build_wasi_       │  │  Manual CMake │ │
│  │ (comprehensive)  │  │simple.sh         │  │  Configuration│ │
│  └────────┬─────────┘  └────────┬─────────┘  └───────┬───────┘ │
│           │                     │                     │          │
│           └─────────────────────┴─────────────────────┘          │
│                                 │                                │
└─────────────────────────────────┼────────────────────────────────┘
                                  │
                                  ▼
┌─────────────────────────────────────────────────────────────────┐
│                      CMake Configuration                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  cmake/wasi-sdk.cmake (Toolchain File)                     │ │
│  │  ┌──────────────────────────────────────────────────────┐  │ │
│  │  │ • Detect WASI_SDK_PATH                               │  │ │
│  │  │ • Set CMAKE_SYSTEM_NAME=WASI                         │  │ │
│  │  │ • Configure compilers (clang, clang++)               │  │ │
│  │  │ • Set sysroot and find paths                         │  │ │
│  │  │ • Initialize compiler/linker flags                   │  │ │
│  │  └──────────────────────────────────────────────────────┘  │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                 │                                │
│                                 ▼                                │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  cmake/CMakeLists.txt                                      │ │
│  │  ┌──────────────────────────────────────────────────────┐  │ │
│  │  │ if (CMAKE_SYSTEM_NAME == "WASI")                     │  │ │
│  │  │   ✅ Enable WebAssembly build                        │  │ │
│  │  │   ➡️  Include onnxruntime_webassembly.cmake          │  │ │
│  │  └──────────────────────────────────────────────────────┘  │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                 │                                │
└─────────────────────────────────┼────────────────────────────────┘
                                  │
                                  ▼
┌─────────────────────────────────────────────────────────────────┐
│          cmake/onnxruntime_webassembly.cmake                      │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │  WASI Configuration                                      │    │
│  │  • Detect WASI vs Emscripten                            │    │
│  │  • Disable unsupported features (JSEP, WebGPU)          │    │
│  │  • Set WASI-specific defines (__wasi__)                 │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                 │                                │
│                    ┌────────────┴────────────┐                   │
│                    │                         │                   │
│                    ▼                         ▼                   │
│  ┌──────────────────────────┐  ┌───────────────────────────┐   │
│  │ Static Library Build     │  │ Executable Build          │   │
│  │ bundle_static_library()  │  │ add_executable()          │   │
│  │                          │  │                           │   │
│  │ • Bundle all libs        │  │ • Link all components     │   │
│  │ • Create .a file         │  │ • Apply WASI linker flags │   │
│  │                          │  │ • Set memory config       │   │
│  │                          │  │ • Enable SIMD             │   │
│  │                          │  │ • Output .wasm            │   │
│  └──────────────────────────┘  └───────────────────────────┘   │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
                                  │
                                  ▼
┌─────────────────────────────────────────────────────────────────┐
│                         Build Output                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│              ort-wasi[-simd][-relaxedsimd][-training].wasm       │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

## Runtime Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                      WASM Binary (.wasm)                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  ONNXRuntime Core                                          │ │
│  │  • Session management                                      │ │
│  │  • Model loading                                           │ │
│  │  • Inference execution                                     │ │
│  │  • Memory management                                       │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                   │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  Optimizations                                             │ │
│  │  • XNNPACK (CPU)                                           │ │
│  │  • SIMD instructions                                       │ │
│  │  • Graph optimizations                                     │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                   │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  Exported Functions                                        │ │
│  │  • All symbols exported (--export-all)                     │ │
│  │  • C API functions                                         │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                   │
└─────────────────────┬───────────────────────────────────────────┘
                      │
                      │ WASI Interface
                      │
        ┌─────────────┼─────────────┐
        │             │             │
        ▼             ▼             ▼
┌──────────────┐ ┌──────────┐ ┌────────────┐
│   Wasmtime   │ │  Wasmer  │ │  Browser   │
│   Runtime    │ │  Runtime │ │ (+polyfill)│
└──────┬───────┘ └─────┬────┘ └──────┬─────┘
       │               │             │
       └───────────────┴─────────────┘
                       │
                       ▼
              ┌─────────────────┐
              │  Host System    │
              │  • File I/O     │
              │  • Memory       │
              │  • Environment  │
              └─────────────────┘
```

## Feature Comparison Matrix

```
┌───────────────────────┬──────────────────┬──────────────────┐
│       Feature         │   WASI-SDK       │   Emscripten     │
├───────────────────────┼──────────────────┼──────────────────┤
│ Output Format         │ .wasm only       │ .js + .wasm      │
│ JavaScript Glue       │ ❌ No            │ ✅ Yes           │
│ Binary Size           │ ✅ Smaller       │ ⚠️  Larger       │
│ Startup Time          │ ✅ Fast          │ ⚠️  Slower       │
│                       │                  │                  │
│ WASI Runtime Support  │ ✅ Native        │ ❌ No            │
│ Browser Support       │ ⚠️  Polyfill     │ ✅ Native        │
│ Node.js Support       │ ✅ Via WASI      │ ✅ Native        │
│                       │                  │                  │
│ Core Inference        │ ✅ Full          │ ✅ Full          │
│ SIMD                  │ ✅ Yes           │ ✅ Yes           │
│ XNNPACK               │ ✅ Yes           │ ✅ Yes           │
│ Training APIs         │ ✅ Yes           │ ✅ Yes           │
│                       │                  │                  │
│ JSEP                  │ ❌ No            │ ✅ Yes           │
│ WebGPU                │ ❌ No            │ ✅ Yes           │
│ WebNN                 │ ❌ No            │ ✅ Yes           │
│ Threading             │ ❌ Limited       │ ✅ pthreads      │
│ Asyncify              │ ❌ No            │ ✅ Yes           │
│                       │                  │                  │
│ Server-Side           │ ✅ Excellent     │ ⚠️  Limited      │
│ Cloud Functions       │ ✅ Excellent     │ ⚠️  Limited      │
│ Edge Computing        │ ✅ Excellent     │ ⚠️  Limited      │
│ Portability           │ ✅ High          │ ⚠️  Medium       │
│ Standardization       │ ✅ W3C WASI      │ ⚠️  De facto     │
└───────────────────────┴──────────────────┴──────────────────┘
```

## Component Dependencies

```
┌─────────────────────────────────────────────────────────────────┐
│                      Build Dependencies                           │
└─────────────────────────────────────────────────────────────────┘
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
        ▼                  ▼                  ▼
┌──────────────┐   ┌──────────────┐   ┌─────────────┐
│  WASI-SDK    │   │    CMake     │   │   Python    │
│  (Required)  │   │  (>=3.18)    │   │  (Build)    │
└──────────────┘   └──────────────┘   └─────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                    Runtime Dependencies                           │
└─────────────────────────────────────────────────────────────────┘
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
        ▼                  ▼                  ▼
┌──────────────┐   ┌──────────────┐   ┌─────────────┐
│  Wasmtime    │   │   Wasmer     │   │  Browser +  │
│  (Option 1)  │   │  (Option 2)  │   │  Polyfill   │
└──────────────┘   └──────────────┘   └─────────────┘
```

## Build Flow Diagram

```
Start
  │
  ▼
[Set WASI_SDK_PATH] ──────────────────────────────────┐
  │                                                    │
  ▼                                                    │
[Run build script]                                     │
  │                                                    │
  ├─→ build_wasi_simple.sh (recommended)              │
  ├─→ build_wasi.sh (detailed)                        │
  └─→ manual cmake (advanced)                         │
  │                                                    │
  ▼                                                    │
[CMake Configure]                                      │
  │                                                    │
  ├─→ Load wasi-sdk.cmake toolchain ─────────────────┤
  │                                                    │
  ├─→ Detect WASI system                              │
  │                                                    │
  ├─→ Include onnxruntime_webassembly.cmake           │
  │                                                    │
  └─→ Configure build options                         │
  │                                                    │
  ▼                                                    │
[CMake Build]                                          │
  │                                                    │
  ├─→ Compile C/C++ sources with clang ──────────────┤
  │                                                    │
  ├─→ Apply SIMD flags (if enabled)                   │
  │                                                    │
  ├─→ Link with wasm-ld ─────────────────────────────┤
  │                                                    │
  └─→ Apply WASI linker options                       │
  │                                                    │
  ▼                                                    │
[Output .wasm file]                                    │
  │                                                    │
  ▼                                                    ▼
ort-wasi[-simd][-relaxedsimd][-training].wasm    [Success]
  │
  ▼
[Run with WASI runtime]
  │
  ├─→ wasmtime run ...
  ├─→ wasmer run ...
  └─→ Node.js + @wasmer/wasi
  │
  ▼
[Inference Execution]
  │
  ▼
End
```

## File Organization

```
onnxruntime-bailey/
│
├── 📁 cmake/
│   ├── 📝 CMakeLists.txt              [MODIFIED - WASI detection]
│   ├── 📝 onnxruntime_webassembly.cmake [MODIFIED - WASI config]
│   └── 📄 wasi-sdk.cmake              [NEW - Toolchain file]
│
├── 🔧 build_wasi.sh                   [NEW - Detailed build]
├── 🔧 build_wasi_simple.sh            [NEW - Simple build]
│
├── 📚 README_WASI.md                  [NEW - Complete guide]
├── 📚 WASI_SDK_MIGRATION.md           [NEW - Technical docs]
├── 📚 WASI_CHANGES_SUMMARY.md         [NEW - Changes list]
├── 📚 WASI_QUICK_REFERENCE.md         [NEW - Quick reference]
└── 📚 WASI_ARCHITECTURE.md            [NEW - This file]
```

## Key Design Decisions

### 1. Dual Support Strategy
```
┌────────────────┐         ┌────────────────┐
│   Emscripten   │         │   WASI-SDK     │
│                │         │                │
│ • Browser-first│         │ • Server-first │
│ • JS interop   │         │ • Portability  │
│ • Feature-rich │         │ • Standards    │
└────────────────┘         └────────────────┘
         │                          │
         └──────────┬───────────────┘
                    │
                    ▼
        onnxruntime_webassembly.cmake
            (Detects build type)
```

### 2. Linker Flags Strategy
```
WASI Linker Flags:
├── --allow-undefined    → Allow runtime imports
├── --export-all         → Export all symbols
├── --no-entry           → No main() required
├── --stack-first        → Optimize stack placement
├── --initial-memory=X   → Set initial memory
└── --max-memory=Y       → Set maximum memory
```

### 3. Memory Configuration
```
Debug Build:
├── Initial: 64MB
└── Maximum: 2GB

Release Build:
├── Initial: 16MB
└── Maximum: 4GB
```

## Integration Patterns

### Pattern 1: Standalone WASI Runtime
```
Your App → wasmtime/wasmer → ort-wasi.wasm → Inference
```

### Pattern 2: Node.js Integration
```
Your App → Node.js → @wasmer/wasi → ort-wasi.wasm → Inference
```

### Pattern 3: Browser Integration
```
Your App → Browser → WASI Polyfill → ort-wasi.wasm → Inference
```

### Pattern 4: Cloud Function
```
Request → Cloud Function → WASI Runtime → ort-wasi.wasm → Response
```

## Performance Considerations

```
┌─────────────────────────────────────────────────────────────┐
│                   Optimization Layers                         │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  Layer 1: Compiler Optimizations (-O3)                       │
│           ↓                                                   │
│  Layer 2: SIMD Instructions (-msimd128 or -mrelaxed-simd)    │
│           ↓                                                   │
│  Layer 3: XNNPACK Operators                                  │
│           ↓                                                   │
│  Layer 4: Graph Optimizations                                │
│           ↓                                                   │
│  Layer 5: Runtime JIT (wasmtime/wasmer)                      │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

## Summary

This WASI-SDK adaptation of ONNXRuntime provides:

✅ **Standardized** WebAssembly builds using W3C WASI
✅ **Portable** across multiple WASI runtimes
✅ **Smaller** binaries without JavaScript glue code
✅ **Server-optimized** for cloud and edge deployments
✅ **Dual support** maintaining Emscripten compatibility
✅ **Well-documented** with comprehensive guides

The architecture maintains all core ONNXRuntime functionality while adapting to WASI's system interface model, making it ideal for server-side machine learning workloads.
