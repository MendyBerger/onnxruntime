#include "onnx_session.h"
#include <iostream>

namespace FlowMark {

ONNXSession::ONNXSession(const std::string& model_path, bool use_cuda)
    : env_(ORT_LOGGING_LEVEL_WARNING, "FlowMark")
    , memory_info_(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)) {
    
    session_options_.SetIntraOpNumThreads(1);
    session_options_.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
    
    // Enable CUDA if requested
    if (use_cuda) {
        OrtCUDAProviderOptions cuda_options;
        session_options_.AppendExecutionProvider_CUDA(cuda_options);
    }
    
    // Create session
    try {
#ifdef _WIN32
        std::wstring wmodel_path(model_path.begin(), model_path.end());
        session_ = std::make_unique<Ort::Session>(env_, wmodel_path.c_str(), session_options_);
#else
        session_ = std::make_unique<Ort::Session>(env_, model_path.c_str(), session_options_);
#endif
        
        // Get input info
        size_t num_input_nodes = session_->GetInputCount();
        for (size_t i = 0; i < num_input_nodes; i++) {
            auto input_name = session_->GetInputNameAllocated(i, allocator_);
            input_names_.push_back(input_name.get());
            
            Ort::TypeInfo type_info = session_->GetInputTypeInfo(i);
            auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
            input_shapes_.push_back(tensor_info.GetShape());
        }
        
        // Get output info
        size_t num_output_nodes = session_->GetOutputCount();
        for (size_t i = 0; i < num_output_nodes; i++) {
            auto output_name = session_->GetOutputNameAllocated(i, allocator_);
            output_names_.push_back(output_name.get());
            
            Ort::TypeInfo type_info = session_->GetOutputTypeInfo(i);
            auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
            output_shapes_.push_back(tensor_info.GetShape());
        }
        
        std::cout << "✓ ONNX Session initialized: " << model_path << std::endl;
        std::cout << "  Inputs: " << num_input_nodes << ", Outputs: " << num_output_nodes << std::endl;
        
    } catch (const Ort::Exception& e) {
        std::cerr << "✗ Failed to create ONNX session: " << e.what() << std::endl;
        session_ = nullptr;
    }
}

ONNXSession::~ONNXSession() = default;

std::vector<float> ONNXSession::run(
    const std::vector<std::pair<std::string, std::vector<float>>>& inputs,
    const std::vector<std::vector<int64_t>>& input_shapes) {
    
    if (!session_) {
        throw std::runtime_error("Session not initialized");
    }
    
    // Create input tensors
    std::vector<Ort::Value> input_tensors;
    std::vector<const char*> input_names_ptr;
    
    for (size_t i = 0; i < inputs.size(); i++) {
        const auto& [name, data] = inputs[i];
        const auto& shape = input_shapes[i];
        
        input_tensors.push_back(Ort::Value::CreateTensor<float>(
            memory_info_,
            const_cast<float*>(data.data()),
            data.size(),
            shape.data(),
            shape.size()
        ));
        
        input_names_ptr.push_back(name.c_str());
    }
    
    // Get output names
    std::vector<const char*> output_names_ptr;
    for (const auto& name : output_names_) {
        output_names_ptr.push_back(name.c_str());
    }
    
    // Run inference
    auto output_tensors = session_->Run(
        Ort::RunOptions{nullptr},
        input_names_ptr.data(),
        input_tensors.data(),
        input_tensors.size(),
        output_names_ptr.data(),
        output_names_ptr.size()
    );
    
    // Extract output data
    float* output_data = output_tensors[0].GetTensorMutableData<float>();
    auto type_info = output_tensors[0].GetTensorTypeAndShapeInfo();
    size_t output_size = type_info.GetElementCount();
    
    return std::vector<float>(output_data, output_data + output_size);
}

std::vector<std::string> ONNXSession::get_input_names() const {
    return input_names_;
}

std::vector<std::string> ONNXSession::get_output_names() const {
    return output_names_;
}

std::vector<std::vector<int64_t>> ONNXSession::get_input_shapes() const {
    return input_shapes_;
}

std::vector<std::vector<int64_t>> ONNXSession::get_output_shapes() const {
    return output_shapes_;
}

} // namespace FlowMark
