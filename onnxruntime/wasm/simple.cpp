#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <cassert>
#include <onnxruntime_cxx_api.h>

int main(int argc, char* argv[]) {
    // Model path: first argument or default.
    // When running under wasmtime: wasmtime run --dir=. ort.wasi.simd.wasm model.ort
    const char* model_path = (argc > 1) ? argv[1] : "simple_model.ort";

    // 1. Initialize ONNX Runtime Environment
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "SimpleInference");

    // 2. Configure Session Options
    Ort::SessionOptions session_options;
    session_options.SetIntraOpNumThreads(1);
    session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

    // Enable WebGPU Execution Provider (only if WEBGPU env var is set)
    if (getenv("USE_WEBGPU")) {
        std::unordered_map<std::string, std::string> webgpu_options;
        session_options.AppendExecutionProvider("WebGPU", webgpu_options);
        std::cout << "WebGPU provider enabled." << std::endl;
    }

    // 3. Create Session and Load Model
    std::cout << "Loading model: " << model_path << std::endl;
    Ort::Session session(env, model_path, session_options);

    // 4. Query input/output names and shapes from the model
    Ort::AllocatorWithDefaultOptions allocator;

    size_t num_inputs = session.GetInputCount();
    size_t num_outputs = session.GetOutputCount();
    std::cout << "Inputs: " << num_inputs << "  Outputs: " << num_outputs << std::endl;

    // Collect input names
    std::vector<std::string> input_name_strs;
    std::vector<const char*> input_names;
    for (size_t i = 0; i < num_inputs; ++i) {
        input_name_strs.push_back(session.GetInputNameAllocated(i, allocator).get());
        input_names.push_back(input_name_strs.back().c_str());
        auto info = session.GetInputTypeInfo(i);
        auto shape = info.GetTensorTypeAndShapeInfo().GetShape();
        std::cout << "  input[" << i << "] = " << input_name_strs.back() << " shape=[";
        for (size_t j = 0; j < shape.size(); ++j) {
            std::cout << shape[j];
            if (j + 1 < shape.size()) std::cout << ",";
        }
        std::cout << "]" << std::endl;
    }

    // Collect output names
    std::vector<std::string> output_name_strs;
    std::vector<const char*> output_names;
    for (size_t i = 0; i < num_outputs; ++i) {
        output_name_strs.push_back(session.GetOutputNameAllocated(i, allocator).get());
        output_names.push_back(output_name_strs.back().c_str());
    }

    // 5. Prepare input: use first input, assume float32 with static shape.
    //    Replace dynamic dims (-1) with 1 for a smoke test.
    auto input_type_info = session.GetInputTypeInfo(0);
    auto input_shape = input_type_info.GetTensorTypeAndShapeInfo().GetShape();
    size_t input_size = 1;
    for (auto& dim : input_shape) {
        if (dim < 0) dim = 1;  // replace dynamic dim with 1
        input_size *= static_cast<size_t>(dim);
    }

    std::vector<float> input_values(input_size, 1.0f);
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memory_info, input_values.data(), input_values.size(),
        input_shape.data(), input_shape.size());

    // 6. Run Inference
    std::cout << "Running inference..." << std::endl;
    auto output_tensors = session.Run(
        Ort::RunOptions{nullptr},
        input_names.data(), &input_tensor, 1,
        output_names.data(), num_outputs);
    std::cout << "Inference complete." << std::endl;

    // 7. Print first output
    assert(!output_tensors.empty() && output_tensors.front().IsTensor());
    float* output_data = output_tensors.front().GetTensorMutableData<float>();
    auto output_shape = output_tensors.front().GetTensorTypeAndShapeInfo().GetShape();

    size_t output_size = 1;
    for (auto dim : output_shape) output_size *= static_cast<size_t>(dim);

    std::cout << "Output[0] (first 10 values): [";
    size_t print_count = std::min(output_size, size_t{10});
    for (size_t i = 0; i < print_count; ++i) {
        std::cout << output_data[i];
        if (i + 1 < print_count) std::cout << ", ";
    }
    if (output_size > 10) std::cout << ", ...";
    std::cout << "]" << std::endl;

    return 0;
}
