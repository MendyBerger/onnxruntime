// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

// #if defined(__wasi__)
// For WASI builds, use wasi-webgpu-headers which provides component model adapters
#include "/media/mendyberger/USB-Card/wasi/wasi-webgpu-headers/webgpu/webgpu.h"
// Then include Dawn's C++ wrapper on top of the wasi-webgpu-headers C implementation
// #include "../../../../../dawn/out/Release/gen/include/dawn/webgpu_cpp.h"
#include "/media/mendyberger/USB-Card/wasi/dawn_webgpu_cpp/webgpu_cpp.h"
// #else
// // #include <webgpu/webgpu_cpp.h>
// #include "../../../../../dawn/out/Release/gen/include/dawn/webgpu_cpp.h"
// #endif

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
