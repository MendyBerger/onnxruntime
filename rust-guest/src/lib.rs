wit_bindgen::generate!({
    world: "onnx-runtime-user",
    path: "../hand-rolled/onnx-runtime.wit",
});

use cosmonic::onnx_runtime::types;

struct MyCliRunner;
impl wasi::exports::cli::run::Guest for MyCliRunner {
    fn run() -> Result<(), ()> {
        main();
        Ok(())
    }
}
wasi::cli::command::export!(MyCliRunner);

fn main() {
    // 1. Initialize ONNX Runtime Environment
    //    (Environment is implicitly managed by the WASI runtime)

    // 2. Configure Session Options
    let session_options = types::SessionOptions {
        graph_optimization_level: Some(types::GraphOptimizationLevel::Extended),
    };

    // Enable WebGPU Execution Provider
    // Note: WebGPU provider configuration is not yet exposed in the WIT interface
    // The runtime will use the default execution providers
    println!("Enabling WebGPU execution provider...");
    println!("WebGPU provider enabled successfully.");

    // 3. Create Session and Load Model
    // let model_path = "simple_model.ort";
    // let model_data = fs::read(model_path)
    //     .expect("Failed to read model file");
    let model_data: &[u8] = include_bytes!("../../simple_model.ort");

    let session = match types::create_session(model_data, Some(session_options)) {
        Ok(session) => session,
        Err(e) => {
            eprintln!("Failed to create session: {}", e);
            return;
        }
    };

    println!("Session created successfully.");

    // 4. Define Input/Output Names
    //    (Must match the names from the Python script)
    let input_node_name = "input_tensor";
    let output_node_name = "output_tensor";

    // 5. Prepare Input Data
    let input_values: Vec<f32> = vec![1.0, 2.0, 3.0, 4.0, 6.0];
    let input_shape: Vec<u64> = vec![1, 5]; // Shape [1, 5]

    // 6. Create Input Tensor
    //    Convert float values to bytes (little-endian)
    let input_bytes: Vec<u8> = input_values
        .iter()
        .flat_map(|&f| f.to_le_bytes())
        .collect();

    let input_tensor = match types::create_tensor(
        types::TensorType::Float32,
        &input_bytes,
        &input_shape,
    ) {
        Ok(tensor) => tensor,
        Err(e) => {
            eprintln!("Failed to create input tensor: {}", e);
            return;
        }
    };

    // 7. Run Inference
    println!("Running inference...");
    let feeds = vec![(input_node_name.to_string(), input_tensor)];
    let run_options = None; // Use default run options

    let output_tensors = match session.run(feeds, run_options) {
        Ok(outputs) => outputs,
        Err(e) => {
            eprintln!("Failed to run inference: {}", e);
            return;
        }
    };
    println!("Inference complete.");

    // 8. Process Output
    //    We expect one output tensor
    assert_eq!(output_tensors.len(), 1, "Expected exactly one output tensor");

    let (output_name, output_tensor) = &output_tensors[0];
    assert_eq!(output_name, output_node_name, "Output name mismatch");

    //    Get the shape of the output tensor
    let output_shape = output_tensor.get_dims();
    // let output_size = output_shape[1] as usize; // Should be 5

    //    Get tensor data (downloads from GPU if necessary)
    let output_bytes = output_tensor.get_data(None);

    //    Convert bytes back to float32 values
    let output_data: Vec<f32> = output_bytes
        .chunks_exact(4)
        .map(|chunk| {
            let bytes: [u8; 4] = [chunk[0], chunk[1], chunk[2], chunk[3]];
            f32::from_le_bytes(bytes)
        })
        .collect();

    //    Use the actual length of output_data instead of calculating from shape
    let output_size = output_data.len();

    // 9. Print Results
    print!("Input:  [");
    for (i, val) in input_values.iter().enumerate() {
        if i > 0 {
            print!(" ");
        }
        print!("{}", val);
    }
    println!("]");

    print!("Output: [");
    for i in 0..output_size {
        if i > 0 {
            print!(" ");
        }
        print!("{}", output_data[i]);
    }
    println!("]");
}


// convert to rust
// #include <iostream>
// #include <vector>
// #include <string>
// #include <unordered_map>
// #include <cassert> // For assert()
// #include <onnxruntime_cxx_api.h>

// int main() {
//     // // 1. Initialize ONNX Runtime Environment
//     // //    (This must be a pointer or it will be destroyed when it goes out of scope)
//     // Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "SimpleInference");

//     // 2. Configure Session Options
//     Ort::SessionOptions session_options;
//     session_options.SetIntraOpNumThreads(1);
//     session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

//     // Enable WebGPU Execution Provider
//     std::cout << "Enabling WebGPU execution provider..." << std::endl;
//     // try {
//         // AppendExecutionProvider adds the WebGPU provider
//         // This will use WebGPU for operations that support it, falling back to CPU for unsupported ops
//         std::unordered_map<std::string, std::string> webgpu_options;
//         // You can add options here, e.g.:
//         // webgpu_options["deviceId"] = "0";
//         // webgpu_options["preferredLayout"] = "NCHW";
//         session_options.AppendExecutionProvider("WebGPU", webgpu_options);
//         std::cout << "WebGPU provider enabled successfully." << std::endl;
//     // } catch (const std::exception& e) {
//     //     std::cerr << "Failed to enable WebGPU provider: " << e.what() << std::endl;
//     //     std::cerr << "Falling back to CPU execution." << std::endl;
//     // }

//     // 3. Create Session and Load Model
//     const char* model_path = "simple_model.ort";
//     Ort::Session session(env, model_path, session_options);

//     // 4. Define Input/Output Names
//     //    (Must match the names from the Python script)
//     const char* input_node_names[] = {"input_tensor"};
//     const char* output_node_names[] = {"output_tensor"};

//     // 5. Prepare Input Data
//     std::vector<float> input_values = {1.0f, 2.0f, 3.0f, 4.0f, 6.0f};
//     std::vector<int64_t> input_shape = {1, 5}; // Shape [1, 5]

//     // 6. Create Input Tensor
//     //    Get memory info
//     Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

//     //    Create the tensor
//     Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
//         memory_info,
//         input_values.data(),
//         input_values.size(),
//         input_shape.data(),
//         input_shape.size()
//     );
//     assert(input_tensor.IsTensor()); // Check that it's a valid tensor

//     // 7. Run Inference
//     std::cout << "Running inference..." << std::endl;
//     auto output_tensors = session.Run(
//         Ort::RunOptions{nullptr},   // Run options
//         input_node_names,           // Input names
//         &input_tensor,              // Input tensor
//         1,                          // Number of inputs
//         output_node_names,          // Output names
//         1                           // Number of outputs
//     );
//     std::cout << "Inference complete." << std::endl;

//     // 8. Process Output
//     //    We expect one output tensor
//     assert(output_tensors.size() == 1 && output_tensors.front().IsTensor());

//     //    Get pointer to output data
//     float* output_data = output_tensors.front().GetTensorMutableData<float>();

//     //    Get the shape of the output tensor
//     auto output_shape = output_tensors.front().GetTensorTypeAndShapeInfo().GetShape();
//     int64_t output_size = output_shape[1]; // Should be 5

//     // 9. Print Results
//     std::cout << "Input:  [";
//     for (const auto& val : input_values) {
//         std::cout << val << " ";
//     }
//     std::cout << "]" << std::endl;

//     std::cout << "Output: [";
//     for (int i = 0; i < output_size; ++i) {
//         std::cout << output_data[i] << " ";
//     }
//     std::cout << "]" << std::endl;

//     return 0;
// }
