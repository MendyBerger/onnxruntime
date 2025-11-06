#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <cassert> // For assert()
#include <onnxruntime_cxx_api.h>

int main() {
    // 1. Initialize ONNX Runtime Environment
    //    (This must be a pointer or it will be destroyed when it goes out of scope)
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "SimpleInference");

    // 2. Configure Session Options
    Ort::SessionOptions session_options;
    session_options.SetIntraOpNumThreads(1);
    session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

    // Enable WebGPU Execution Provider
    std::cout << "Enabling WebGPU execution provider..." << std::endl;
    // try {
        // AppendExecutionProvider adds the WebGPU provider
        // This will use WebGPU for operations that support it, falling back to CPU for unsupported ops
        std::unordered_map<std::string, std::string> webgpu_options;
        // You can add options here, e.g.:
        // webgpu_options["deviceId"] = "0";
        // webgpu_options["preferredLayout"] = "NCHW";
        session_options.AppendExecutionProvider("WebGPU", webgpu_options);
        std::cout << "WebGPU provider enabled successfully." << std::endl;
    // } catch (const std::exception& e) {
    //     std::cerr << "Failed to enable WebGPU provider: " << e.what() << std::endl;
    //     std::cerr << "Falling back to CPU execution." << std::endl;
    // }

    // 3. Create Session and Load Model
    const char* model_path = "simple_model.ort";
    Ort::Session session(env, model_path, session_options);

    // 4. Define Input/Output Names
    //    (Must match the names from the Python script)
    const char* input_node_names[] = {"input_tensor"};
    const char* output_node_names[] = {"output_tensor"};

    // 5. Prepare Input Data
    std::vector<float> input_values = {1.0f, 2.0f, 3.0f, 4.0f, 6.0f};
    std::vector<int64_t> input_shape = {1, 5}; // Shape [1, 5]

    // 6. Create Input Tensor
    //    Get memory info
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    //    Create the tensor
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memory_info,
        input_values.data(),
        input_values.size(),
        input_shape.data(),
        input_shape.size()
    );
    assert(input_tensor.IsTensor()); // Check that it's a valid tensor

    // 7. Run Inference
    std::cout << "Running inference..." << std::endl;
    auto output_tensors = session.Run(
        Ort::RunOptions{nullptr},   // Run options
        input_node_names,           // Input names
        &input_tensor,              // Input tensor
        1,                          // Number of inputs
        output_node_names,          // Output names
        1                           // Number of outputs
    );
    std::cout << "Inference complete." << std::endl;

    // 8. Process Output
    //    We expect one output tensor
    assert(output_tensors.size() == 1 && output_tensors.front().IsTensor());

    //    Get pointer to output data
    float* output_data = output_tensors.front().GetTensorMutableData<float>();

    //    Get the shape of the output tensor
    auto output_shape = output_tensors.front().GetTensorTypeAndShapeInfo().GetShape();
    int64_t output_size = output_shape[1]; // Should be 5

    // 9. Print Results
    std::cout << "Input:  [";
    for (const auto& val : input_values) {
        std::cout << val << " ";
    }
    std::cout << "]" << std::endl;

    std::cout << "Output: [";
    for (int i = 0; i < output_size; ++i) {
        std::cout << output_data[i] << " ";
    }
    std::cout << "]" << std::endl;

    return 0;
}
