// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#if defined(__wasm__)

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#else
#include <memory>
#endif

#include "core/framework/tensor.h"
#include "core/providers/webgpu/external_data_loader.h"
#if !defined(__EMSCRIPTEN__)
#include "core/providers/webgpu/webgpu_context.h"
#endif

namespace onnxruntime {
namespace webgpu {

bool ExternalDataLoader::CanLoad(const OrtMemoryInfo& target_memory_info) const {
  return target_memory_info.device.Type() == OrtDevice::CPU ||
         (target_memory_info.device.Type() == OrtDevice::GPU && target_memory_info.name == WEBGPU_BUFFER);
}

common::Status ExternalDataLoader::LoadTensor(const Env& env,
                                              const std::filesystem::path& data_file_path,
                                              FileOffsetType data_offset,
                                              SafeInt<size_t> data_length,
                                              Tensor& tensor) const {
#if defined(__EMSCRIPTEN__)
  // Emscripten: use JS interop to load from Module.MountedFiles
  ExternalDataLoadType load_type;
  if (tensor.Location().device.Type() == OrtDevice::CPU) {
    load_type = ExternalDataLoadType::CPU;
  } else if (tensor.Location().device.Type() == OrtDevice::GPU &&
             tensor.Location().name == WEBGPU_BUFFER) {
    load_type = ExternalDataLoadType::WEBGPU_BUFFER;
  } else {
    return ORT_MAKE_STATUS(ONNXRUNTIME, FAIL, "Unsupported tensor location: ", tensor.Location().ToString());
  }

  return LoadWebAssemblyExternalData(env, data_file_path, data_offset, data_length, load_type, tensor.MutableDataRaw());
#else
  // WASI: use file I/O
  if (tensor.Location().device.Type() == OrtDevice::CPU) {
    gsl::span<char> buffer(static_cast<char*>(tensor.MutableDataRaw()), data_length);
    ORT_RETURN_IF_ERROR(env.ReadFileIntoBuffer(data_file_path.native().c_str(), data_offset, data_length, buffer));
  } else if (tensor.Location().device.Type() == OrtDevice::GPU &&
             tensor.Location().name == WEBGPU_BUFFER) {
    // Read into CPU buffer first, then upload to GPU.
    auto cpu_buf = std::make_unique<char[]>(data_length);
    gsl::span<char> buffer(cpu_buf.get(), data_length);
    ORT_RETURN_IF_ERROR(env.ReadFileIntoBuffer(data_file_path.native().c_str(), data_offset, data_length, buffer));
    // wgpu::Buffer(WGPUBuffer) calls AddRef; destructor calls Release — ORT retains ownership.
    wgpu::Buffer gpu_buffer(static_cast<WGPUBuffer>(tensor.MutableDataRaw()));
    WebGpuContextFactory::GetContext(0).Device().GetQueue().WriteBuffer(gpu_buffer, 0, cpu_buf.get(), data_length);
  } else {
    return ORT_MAKE_STATUS(ONNXRUNTIME, FAIL, "Unsupported tensor location: ", tensor.Location().ToString());
  }

  return Status::OK();
#endif
}

}  // namespace webgpu
}  // namespace onnxruntime

#endif
