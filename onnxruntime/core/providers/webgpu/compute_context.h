// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "core/providers/webgpu/webgpu_external_header.h"

#include <utility>

#include "core/framework/execution_provider.h"
#include "core/providers/webgpu/webgpu_execution_provider.h"

#include "core/providers/webgpu/program.h"
#include "core/providers/webgpu/webgpu_context.h"
#include "core/framework/op_kernel.h"

namespace onnxruntime {

class Tensor;

namespace webgpu {

class WebGpuContext;
class BufferManager;

class ComputeContext {
 public:
  ComputeContext(OpKernelContext& kernel_context, const WebGpuExecutionProvider& ep);

  virtual ~ComputeContext() = default;

  //
  // Get various information from the context.
  //

  inline const wgpu::AdapterInfo& AdapterInfo() const {
    return webgpu_context_.AdapterInfo();
  }
  inline const wgpu::Limits& DeviceLimits() const {
    return webgpu_context_.DeviceLimits();
  }
  inline bool HasFeature(wgpu::FeatureName feature) const {
    return webgpu_context_.DeviceHasFeature(feature);
  }
  inline bool IsGraphCaptureEnabled() const {
    return ep_.IsGraphCaptureEnabled();
  }
#if !defined(__wasm__)
  inline const wgpu::AdapterPropertiesSubgroupMatrixConfigs& SubgroupMatrixConfigs() const {
    return webgpu_context_.SubgroupMatrixConfigs();
  }
#endif

  //
  // Get the kernel context.
  //
  inline OpKernelContext& KernelContext() {
    return kernel_context_;
  }

  //
  // Get the logger.
  //
  inline const logging::Logger& Logger() const {
    return kernel_context_.Logger();
  }

  //
  // Get input tensor.
  //
  template <typename T = onnxruntime::Tensor>
  inline const T* Input(int index) const {
    return kernel_context_.Input<T>(index);
  }

  //
  // Get input count.
  //
  inline int InputCount() const {
    return kernel_context_.InputCount();
  }

  //
  // Set output tensor.
  //
  template <typename TensorShapeType>
  inline Tensor* Output(int index, TensorShapeType&& shape) {
    return kernel_context_.Output(index, std::forward<TensorShapeType>(shape));
  }

  //
  // Get output count.
  //
  inline int OutputCount() const {
    return kernel_context_.OutputCount();
  }

  //
  // Create CPU tensor.
  //
  // This method creates a tensor of the given data type and shape, using the CPU allocator.
  // The tensor owns the underlying CPU memory buffer.
  //
  template <typename TensorShapeType>
  Tensor CreateCPUTensor(MLDataType data_type, TensorShapeType&& shape) {
    AllocatorPtr allocator;
    ORT_THROW_IF_ERROR(kernel_context_.GetTempSpaceCPUAllocator(&allocator));
    return {data_type, std::forward<TensorShapeType>(shape), allocator};
  }

  //
  // Create GPU tensor.
  //
  // This method creates a tensor of the given data type and shape, using the WebGPU allocator.
  // The tensor owns the underlying WebGPU storage buffer.
  //
  template <typename TensorShapeType>
  Tensor CreateGPUTensor(MLDataType data_type, TensorShapeType&& shape) {
    AllocatorPtr allocator;
    ORT_THROW_IF_ERROR(kernel_context_.GetTempSpaceAllocator(&allocator));
    return {data_type, std::forward<TensorShapeType>(shape), allocator};
  }

  // // Add to ComputeContext class in compute_context.h
  // //
  // // Convert float16 tensor to float32 if f16 feature is not available
  // //
  // template <typename TensorShapeType>
  // Tensor ConvertFloat16ToFloat32IfNeeded(const Tensor* input_tensor, TensorShapeType&& shape) {
  //   if (input_tensor->GetElementType() == DataTypeImpl::GetType<MLFloat16>() &&
  //       !HasFeature(wgpu::FeatureName::ShaderF16)) {
  //     // Create a float32 tensor
  //     Tensor float32_tensor = CreateGPUTensor(DataTypeImpl::GetType<float>(), std::forward<TensorShapeType>(shape));

  //     // Convert f16 -> f32 (this would need a conversion kernel or CPU conversion)
  //     // For now, this is a placeholder - you'd need to implement the actual conversion
  //     return float32_tensor;
  //   }
  //   return *input_tensor; // Return original if no conversion needed
  // }

  //
  // Run a compute shader program.
  //
  inline Status RunProgram(ProgramBase& program) {
    return webgpu_context_.Run(*this, program);
  }

  //
  // Get the buffer manager from the GPU allocator.
  //
  const webgpu::BufferManager& BufferManager() const;

  //
  // Push error scope.
  //
  // This is useful only when "skip_validation" is not set.
  //
  void PushErrorScope();

  //
  // Pop error scope.
  //
  // This is useful only when "skip_validation" is not set.
  //
  Status PopErrorScope();

 protected:
  WebGpuContext& webgpu_context_;
  OpKernelContext& kernel_context_;
  const WebGpuExecutionProvider& ep_;
};

}  // namespace webgpu
}  // namespace onnxruntime
