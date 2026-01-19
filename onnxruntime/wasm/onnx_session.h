#pragma once

#include <string>
#include <vector>
#include <memory>
#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>

namespace FlowMark {

class ONNXSession {
public:
    // Constructor
    ONNXSession(const std::string& model_path, bool use_cuda = false);
    
    // Destructor
    ~ONNXSession();
    
    // Run inference
    // @param inputs: map of input name to tensor data
    // @return: map of output name to tensor data
    std::vector<float> run(const std::vector<std::pair<std::string, std::vector<float>>>& inputs,
                           const std::vector<std::vector<int64_t>>& input_shapes);
    
    // Get input/output info
    std::vector<std::string> get_input_names() const;
    std::vector<std::string> get_output_names() const;
    std::vector<std::vector<int64_t>> get_input_shapes() const;
    std::vector<std::vector<int64_t>> get_output_shapes() const;
    
    // Check if session is initialized
    bool is_initialized() const { return session_ != nullptr; }

private:
    Ort::Env env_;
    Ort::SessionOptions session_options_;
    std::unique_ptr<Ort::Session> session_;
    Ort::MemoryInfo memory_info_;
    Ort::AllocatorWithDefaultOptions allocator_;
    
    std::vector<std::string> input_names_;
    std::vector<std::string> output_names_;
    std::vector<std::vector<int64_t>> input_shapes_;
    std::vector<std::vector<int64_t>> output_shapes_;
};

} // namespace FlowMark
