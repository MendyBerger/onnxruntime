# Exception Handling Disabled for WASI Preview 2

## Overview

WASI Preview 2 uses the WebAssembly Component Model, which does not support importing C++ exception handling functions like `__cxa_allocate_exception`, `__cxa_throw`, etc. Therefore, all exception handling must be disabled when building ONNXRuntime for WASI Preview 2.

## What Was Changed

### 1. Toolchain Configuration (`cmake/wasi-sdk.cmake`)

**Compiler Flags Added:**
```cmake
-fno-exceptions              # Disable C++ exception handling
-fno-rtti                    # Disable runtime type information
-fno-unwind-tables           # Remove exception unwinding tables
-fno-asynchronous-unwind-tables  # Remove async unwinding support
```

**Compile Definitions Added:**
```cmake
ORT_NO_EXCEPTIONS           # ONNXRuntime: use return-code based errors
ONNX_NO_EXCEPTIONS          # ONNX: disable exceptions
MLAS_NO_EXCEPTION           # MLAS: disable exceptions
JSON_NOEXCEPTION            # nlohmann/json: disable exceptions
```

### 2. Build Scripts Updated

**`build_wasi_simple.sh` and `build_wasi.sh`:**
```bash
-Donnxruntime_MINIMAL_BUILD=ON          # Required for exception disabling
-Donnxruntime_DISABLE_EXCEPTIONS=ON     # Disable exceptions in ORT
-Donnxruntime_DISABLE_RTTI=ON           # Disable RTTI
-DONNX_DISABLE_EXCEPTIONS=ON            # Disable exceptions in ONNX
```

### 3. WebAssembly CMake Configuration

**`cmake/onnxruntime_webassembly.cmake`:**
- Updated to show warning if exception catching is requested
- Documents that exceptions cannot be enabled for WASI Preview 2

## How Error Handling Works Without Exceptions

When `ORT_NO_EXCEPTIONS` is defined, ONNXRuntime uses return-code based error handling:

### Before (With Exceptions):
```cpp
try {
    session.Run(...);
} catch (const Ort::Exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
}
```

### After (Without Exceptions):
```cpp
Ort::Status status = session.Run(...);
if (!status.IsOK()) {
    std::cerr << "Error: " << status.ErrorMessage() << std::endl;
}
```

## Impact on Dependencies

All major dependencies are configured to build without exceptions:

| Dependency | Configuration | Notes |
|------------|--------------|-------|
| **Protobuf** | `GOOGLE_PROTOBUF_NO_THREADLOCAL=1` | No thread-local storage |
| **ONNX** | `ONNX_NO_EXCEPTIONS=1` | Uses error codes |
| **MLAS** | `MLAS_NO_EXCEPTION=1` | Math library without exceptions |
| **nlohmann/json** | `JSON_NOEXCEPTION=1` | JSON parsing without exceptions |
| **Abseil** | `ABSL_HAVE_THREAD_LOCAL=0` | No thread-local storage |

## Why This Is Required

The WASI Preview 2 Component Model has strict limitations:

1. **No Exception Imports**: Cannot import functions like:
   - `env::__cxa_allocate_exception`
   - `env::__cxa_throw`
   - `env::__cxa_begin_catch`
   - `env::__cxa_end_catch`

2. **Component Sandboxing**: Components must be fully self-contained

3. **Interface Types**: All cross-component calls must use component model types

## Verification

To verify exceptions are properly disabled, check the build output:

```bash
# Should see these messages:
WASI-SDK toolchain loaded (Preview 2)
  Target: wasm32-wasip2
  Exceptions: DISABLED (component model requirement)

# Check the final binary doesn't have exception imports:
wasm-objdump -x build_wasi/onnxruntime_webassembly.wasm | grep -i exception
# (should have no output)
```

## Performance Impact

Disabling exceptions actually provides several benefits:

1. **Smaller Binary Size**: No exception unwinding tables (~10-20% size reduction)
2. **Faster Execution**: No exception handling overhead
3. **More Predictable**: No hidden control flow from exceptions
4. **Better for WebAssembly**: Aligns with WASM's efficient execution model

## Additional WASI Restrictions

### Dynamic Library Loading Disabled

In addition to exceptions, WASI Preview 2 doesn't support dynamic library loading (`dlopen`, `dlclose`, `dlsym`, `dlerror`). The following changes were made:

**File: `onnxruntime/core/platform/posix/env.cc`**

All dynamic library functions now return `NOT_IMPLEMENTED` status on WASI:
- `LoadDynamicLibrary()` - Cannot load external libraries
- `UnloadDynamicLibrary()` - Cannot unload libraries
- `GetSymbolFromLibrary()` - Cannot lookup symbols

This affects:
- Custom operator plugins
- Execution provider plugins
- Shared library extensions

For WASI builds, all functionality must be statically linked at compile time.

## Future Considerations

- **WASI Preview 3+**: May add component-model-compatible exception handling
- **Exception Proposal**: WebAssembly exception handling proposal is separate from component model
- **Hybrid Approach**: Possible future support for exceptions within a single component
- **Dynamic Linking**: WASI may eventually support component-model-based dynamic linking

## Additional Resources

- [WASI Component Model](https://github.com/WebAssembly/component-model)
- [ONNXRuntime Minimal Build Guide](https://onnxruntime.ai/docs/build/custom.html)
- [WebAssembly Exception Handling Proposal](https://github.com/WebAssembly/exception-handling)
