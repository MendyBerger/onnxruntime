// Minimal WASI entry point for ONNXRuntime.
// Loads a model, runs inference, prints output.
// Usage: wasmtime run --dir=. ort-wasi.wasm -- model.ort

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <onnxruntime_cxx_api.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << (argc > 0 ? argv[0] : "ort-wasi") << " <model.ort>" << std::endl;
        return 1;
    }

    const char* model_path = argv[1];

    // Initialize ORT
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "OrtWasi");
    Ort::SessionOptions session_options;
    session_options.SetIntraOpNumThreads(1);
    session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

    // Enable WebGPU if requested
    if (getenv("USE_WEBGPU")) {
        std::unordered_map<std::string, std::string> webgpu_options;
        webgpu_options["preferredLayout"] = "NCHW";
        session_options.AppendExecutionProvider("WebGPU", webgpu_options);
        std::cout << "WebGPU execution provider enabled" << std::endl;
    }

    // Load model
    Ort::Session session(env, model_path, session_options);
    Ort::AllocatorWithDefaultOptions allocator;

    // Print model info
    size_t num_inputs = session.GetInputCount();
    size_t num_outputs = session.GetOutputCount();
    std::cout << "Model: " << model_path << std::endl;
    std::cout << "Inputs: " << num_inputs << ", Outputs: " << num_outputs << std::endl;

    for (size_t i = 0; i < num_inputs; i++) {
        auto name = session.GetInputNameAllocated(i, allocator);
        auto shape = session.GetInputTypeInfo(i).GetTensorTypeAndShapeInfo().GetShape();
        std::cout << "  Input " << i << ": " << name.get() << " [";
        for (size_t j = 0; j < shape.size(); j++) {
            std::cout << shape[j];
            if (j < shape.size() - 1) std::cout << ", ";
        }
        std::cout << "]" << std::endl;
    }

    // Create dummy input tensors (zeros) matching model shapes
    Ort::MemoryInfo mem = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    std::vector<std::vector<float>> input_data_storage(num_inputs);
    std::vector<Ort::Value> input_tensors;
    std::vector<const char*> input_names(num_inputs);
    std::vector<Ort::AllocatedStringPtr> input_name_ptrs;

    for (size_t i = 0; i < num_inputs; i++) {
        input_name_ptrs.push_back(session.GetInputNameAllocated(i, allocator));
        input_names[i] = input_name_ptrs.back().get();

        auto shape = session.GetInputTypeInfo(i).GetTensorTypeAndShapeInfo().GetShape();
        int64_t elem_count = 1;
        for (auto d : shape) elem_count *= (d > 0 ? d : 1);

        input_data_storage[i].resize(static_cast<size_t>(elem_count), 0.0f);
        input_tensors.push_back(Ort::Value::CreateTensor<float>(
            mem, input_data_storage[i].data(), input_data_storage[i].size(),
            shape.data(), shape.size()));
    }

    // Output names
    std::vector<const char*> output_names(num_outputs);
    std::vector<Ort::AllocatedStringPtr> output_name_ptrs;
    for (size_t i = 0; i < num_outputs; i++) {
        output_name_ptrs.push_back(session.GetOutputNameAllocated(i, allocator));
        output_names[i] = output_name_ptrs.back().get();
    }

    // Run inference
    std::cout << "Running inference..." << std::endl;
    auto outputs = session.Run(Ort::RunOptions{nullptr},
                               input_names.data(), input_tensors.data(), num_inputs,
                               output_names.data(), num_outputs);
    std::cout << "Inference complete." << std::endl;

    // Print first few output values
    for (size_t i = 0; i < outputs.size(); i++) {
        auto shape = outputs[i].GetTensorTypeAndShapeInfo().GetShape();
        float* data = outputs[i].GetTensorMutableData<float>();
        int64_t total = 1;
        for (auto d : shape) total *= d;

        std::cout << "  Output " << i << ": [";
        for (int64_t j = 0; j < std::min(total, (int64_t)10); j++) {
            if (j > 0) std::cout << ", ";
            std::cout << data[j];
        }
        if (total > 10) std::cout << ", ...";
        std::cout << "]" << std::endl;
    }

    return 0;
}
