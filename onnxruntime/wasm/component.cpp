#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <cassert> // For assert()
#include <onnxruntime_cxx_api.h>

#include "../../hand-rolled/onnx_runtime_impl.h"


_Noreturn void abort_1(void){ abort(); }
_Noreturn void abort_2(void){ abort(); }
_Noreturn void abort_3(void){ abort(); }
_Noreturn void abort_4(void){ abort(); }
_Noreturn void abort_5(void){ abort(); }
_Noreturn void abort_6(void){ abort(); }
_Noreturn void abort_7(void){ abort(); }
_Noreturn void abort_8(void){ abort(); }

typedef struct exports_cosmonic_onnx_runtime_types_session_t {
    Ort::Session *session;
    Ort::Env *env;  // Add this to keep the environment alive
} exports_cosmonic_onnx_runtime_types_session_t;

typedef struct exports_cosmonic_onnx_runtime_types_tensor_t {
    Ort::Value *tensor;
    void *data_buffer;  // Add this to track the copied data buffer
} exports_cosmonic_onnx_runtime_types_tensor_t;

GraphOptimizationLevel graph_optimization_level_wit_to_cpp(exports_cosmonic_onnx_runtime_types_graph_optimization_level_t graph_optimization_level);
onnx_runtime_impl_string_t string_cpp_to_wasi(const char* string_cpp);
const char* string_wasi_to_cpp(onnx_runtime_impl_string_t string_wasi);

bool exports_cosmonic_onnx_runtime_types_create_session(onnx_runtime_impl_list_u8_t *model_data, exports_cosmonic_onnx_runtime_types_session_options_t *maybe_options, exports_cosmonic_onnx_runtime_types_own_session_t *ret, exports_cosmonic_onnx_runtime_types_error_t *err) {
    if (model_data == NULL) {
        abort();
    }

    // Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "SimpleInference"); // TODO: what args here?
    Ort::Env *env = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "SimpleInference");
    if (env == nullptr) {
        abort();
    }


    Ort::SessionOptions session_options;

    // session_options.SetIntraOpNumThreads(1);
    // session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);


    // hard code to WebGPU for now
    std::unordered_map<std::string, std::string> webgpu_options;
    // session_options.AppendExecutionProvider("WebGPU", webgpu_options);
    // hard code to 1 for now
    session_options.SetIntraOpNumThreads(1);

    if (maybe_options != NULL) {
        if (maybe_options->graph_optimization_level.is_some) {
            session_options.SetGraphOptimizationLevel(graph_optimization_level_wit_to_cpp(maybe_options->graph_optimization_level.val));
        }
    }


    // Ort::Session ort_session(env, model_data->ptr, model_data->len, session_options);
    Ort::Session *ort_session = new Ort::Session(*env, model_data->ptr, model_data->len, session_options);


    if (ort_session == nullptr) {
        abort_5();
    }



    // exports_cosmonic_onnx_runtime_types_session_t session = {
    exports_cosmonic_onnx_runtime_types_session_t *session = new exports_cosmonic_onnx_runtime_types_session_t{
        .session = ort_session,
        .env = env
    };
    *ret = exports_cosmonic_onnx_runtime_types_session_new(session);
    return true;
}

// bool exports_cosmonic_onnx_runtime_types_method_session_run(exports_cosmonic_onnx_runtime_types_borrow_session_t self, exports_cosmonic_onnx_runtime_types_list_tuple2_string_own_tensor_t *feeds, exports_cosmonic_onnx_runtime_types_run_options_t *maybe_options, exports_cosmonic_onnx_runtime_types_list_tuple2_string_own_tensor_t *ret, exports_cosmonic_onnx_runtime_types_error_t *err) {
//     if (self == NULL || feeds == NULL) {
//         abort();
//     }

//     Ort::RunOptions run_options = Ort::RunOptions(nullptr);
//     if (maybe_options != NULL) {
//         // TODO: implement all the options
//         if (maybe_options->log_severity_level.is_some) {
//             run_options.SetRunLogSeverityLevel(maybe_options->log_severity_level.val);
//         }
//         if (maybe_options->log_verbosity_level.is_some) {
//             run_options.SetRunLogVerbosityLevel(maybe_options->log_verbosity_level.val);
//         }
//     }

//     std::vector<Ort::Value> input_tensors;
//     std::vector<const char*> input_names;
//     for (size_t i = 0; i < feeds->len; i++) {
//         // exports_cosmonic_onnx_runtime_types_tuple2_string_own_tensor_t *feed = &feeds->ptr[i];
//         // exports_cosmonic_onnx_runtime_types_own_tensor_t *tensor = feed->tensor->f1;
//         exports_cosmonic_onnx_runtime_types_tensor_t *tensor = exports_cosmonic_onnx_runtime_types_tensor_rep(feeds->ptr[i].f1);
//         // input_tensors.push_back(*tensor->tensor);
//         input_tensors.push_back(std::move(*tensor->tensor));  // TODO:
//         const char* name = string_wasi_to_cpp(feeds->ptr[i].f0);
//         input_names.push_back(name);
//     }

//     if (input_tensors.size() != input_names.size()) {
//         abort();
//     }

//     // TODO: check if this logic is correct
//     std::vector<Ort::Value> output_tensors;
//     std::vector<const char*> output_names;
//     for (size_t i = 0; i < ret->len; i++) {
//         const char* name = string_wasi_to_cpp(ret->ptr[i].f0);
//         output_names.push_back(name);
//     }

//     output_tensors = self->session->Run(run_options, input_names.data(), input_tensors.data(), input_names.size(), output_names.data(), output_names.size());
//     return true;
// }


bool exports_cosmonic_onnx_runtime_types_method_session_run(exports_cosmonic_onnx_runtime_types_borrow_session_t self, exports_cosmonic_onnx_runtime_types_list_tuple2_string_own_tensor_t *feeds, exports_cosmonic_onnx_runtime_types_run_options_t *maybe_options, exports_cosmonic_onnx_runtime_types_list_tuple2_string_own_tensor_t *ret, exports_cosmonic_onnx_runtime_types_error_t *err) {
    if (self == NULL || feeds == NULL) {
        abort();
    }

    Ort::RunOptions run_options = Ort::RunOptions(nullptr);
    if (maybe_options != NULL) {
        // TODO: implement all the options
        if (maybe_options->log_severity_level.is_some) {
            run_options.SetRunLogSeverityLevel(maybe_options->log_severity_level.val);
        }
        if (maybe_options->log_verbosity_level.is_some) {
            run_options.SetRunLogVerbosityLevel(maybe_options->log_verbosity_level.val);
        }
    }

    std::vector<Ort::Value> input_tensors;
    std::vector<const char*> input_names;
    std::cout << "exports_cosmonic_onnx_runtime_types_method_session_run 3" << std::endl;
    for (size_t i = 0; i < feeds->len; i++) {
        exports_cosmonic_onnx_runtime_types_tensor_t *tensor = exports_cosmonic_onnx_runtime_types_tensor_rep(feeds->ptr[i].f1);


        if (tensor == NULL) {
            abort_1();
        }

        if (tensor->tensor == NULL) {
            abort_2();
        }

        if (!tensor->tensor->IsTensor()) {
            abort_3();
        }

        // Validate tensor before using it
        if (tensor == NULL || tensor->tensor == NULL || !tensor->tensor->IsTensor()) {
            abort();  // Invalid tensor
        }

        input_tensors.push_back(std::move(*tensor->tensor));
        const char* name = string_wasi_to_cpp(feeds->ptr[i].f0);
        input_names.push_back(name);
    }

    std::cout << "exports_cosmonic_onnx_runtime_types_method_session_run 4" << std::endl;
    if (input_tensors.size() != input_names.size()) {
        abort();
    }

    // // TODO: check if this logic is correct
    // std::vector<Ort::Value> output_tensors;
    // std::vector<const char*> output_names;
    // for (size_t i = 0; i < ret->len; i++) {
    //     const char* name = string_wasi_to_cpp(ret->ptr[i].f0);
    //     output_names.push_back(name);
    // }

    // output_tensors = self->session->Run(run_options, input_names.data(), input_tensors.data(), input_names.size(), output_names.data(), output_names.size());

    std::cout << "exports_cosmonic_onnx_runtime_types_method_session_run 4" << std::endl;
    if (input_tensors.size() != input_names.size()) {
        abort();
    }

    // Get output names from the session instead of reading from ret
    std::vector<std::string> output_names_vec = self->session->GetOutputNames();
    if (output_names_vec.empty()) {
        abort();  // No outputs in the model
    }

    std::vector<const char*> output_names;
    std::vector<char*> output_names_allocated;  // Track allocated strings for cleanup
    for (const auto& name : output_names_vec) {
        char* name_cstr = new char[name.length() + 1];
        strcpy(name_cstr, name.c_str());
        output_names.push_back(name_cstr);
        output_names_allocated.push_back(name_cstr);
    }

    // Run inference
    std::vector<Ort::Value> output_tensors = self->session->Run(
        run_options,
        input_names.data(),
        input_tensors.data(),
        input_names.size(),
        output_names.data(),
        output_names.size()
    );

    // Clean up allocated output name strings
    for (char* name : output_names_allocated) {
        delete[] name;
    }

    // Populate ret with the results
    ret->len = output_tensors.size();
    ret->ptr = new exports_cosmonic_onnx_runtime_types_tuple2_string_own_tensor_t[ret->len];

    // for (size_t i = 0; i < output_tensors.size(); i++) {
    //     // Convert output name to WASI string
    //     ret->ptr[i].f0 = string_cpp_to_wasi(output_names_vec[i].c_str());

    //     // // Wrap the output tensor
    //     // Ort::Value *ort_value = new Ort::Value(std::move(output_tensors[i]));
    //     // exports_cosmonic_onnx_runtime_types_tensor_t *tensor = new exports_cosmonic_onnx_runtime_types_tensor_t{
    //     //     .tensor = ort_value,
    //     //     .data_buffer = nullptr  // Output tensors don't need a data buffer
    //     // };

    //     Ort::Value& output_value = output_tensors[i];
    //     auto shape_info = output_value.GetTensorTypeAndShapeInfo();
    //     size_t element_count = shape_info.GetElementCount();

    //     // Allocate and copy output data
    //     float* output_buffer = new float[element_count];
    //     memcpy(output_buffer, output_value.GetTensorData<float>(), element_count * sizeof(float));

    //     // Create new CPU tensor with copied data
    //     Ort::MemoryInfo cpu_memory = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    //     Ort::Value *ort_value = new Ort::Value(Ort::Value::CreateTensor<float>(
    //         cpu_memory,
    //         output_buffer,
    //         element_count,
    //         shape_info.GetShape().data(),
    //         shape_info.GetShape().size()
    //     ));

    //     exports_cosmonic_onnx_runtime_types_tensor_t *tensor = new exports_cosmonic_onnx_runtime_types_tensor_t{
    //         .tensor = ort_value,
    //         .data_buffer = output_buffer  // Track the buffer so it can be freed later
    //     };

    //     ret->ptr[i].f1 = exports_cosmonic_onnx_runtime_types_tensor_new(tensor);
    // }

    for (size_t i = 0; i < output_tensors.size(); i++) {
        ret->ptr[i].f0 = string_cpp_to_wasi(output_names_vec[i].c_str());

        // CRITICAL: Copy output data from GPU to CPU memory
        auto& output_value = output_tensors[i];
        auto type_info = output_value.GetTensorTypeAndShapeInfo();
        size_t element_count = type_info.GetElementCount();
        auto element_type = type_info.GetElementType();

        // Allocate CPU buffer and copy data
        float* cpu_buffer = new float[element_count];
        const float* gpu_data = output_value.GetTensorData<float>();
        memcpy(cpu_buffer, gpu_data, element_count * sizeof(float));

        // Create new CPU tensor with copied data
        Ort::MemoryInfo cpu_memory = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value* cpu_tensor = new Ort::Value(Ort::Value::CreateTensor<float>(
            cpu_memory,
            cpu_buffer,
            element_count,
            type_info.GetShape().data(),
            type_info.GetShape().size()
        ));

        exports_cosmonic_onnx_runtime_types_tensor_t *tensor = new exports_cosmonic_onnx_runtime_types_tensor_t{
            .tensor = cpu_tensor,
            .data_buffer = cpu_buffer  // Track buffer for cleanup
        };
        ret->ptr[i].f1 = exports_cosmonic_onnx_runtime_types_tensor_new(tensor);
    }



    return true;
}

void exports_cosmonic_onnx_runtime_types_method_session_get_inputs(exports_cosmonic_onnx_runtime_types_borrow_session_t self, exports_cosmonic_onnx_runtime_types_list_tuple2_string_tensor_type_t *ret)
{
    if (self == NULL || ret == NULL) {
        abort();
    }

    std::vector<std::string> input_names = self->session->GetInputNames();
    std::cout << "C++ side input_names count: " << input_names.size() << std::endl;
    for (const auto& name : input_names) {
        std::cout << "  C++ input: " << name << std::endl;
    }
    // std::cout << "input_names: " << input_names << std::endl;
    if (input_names.empty()) {
        abort();  // No inputs in the model
    }

    ret->len = input_names.size();
    ret->ptr = new exports_cosmonic_onnx_runtime_types_tuple2_string_tensor_type_t[ret->len];

    for (size_t i = 0; i < input_names.size(); i++) {
        ret->ptr[i].f0 = string_cpp_to_wasi(input_names[i].c_str());
        ret->ptr[i].f1 = EXPORTS_COSMONIC_ONNX_RUNTIME_TYPES_TENSOR_TYPE_FLOAT32;
    }
}



bool exports_cosmonic_onnx_runtime_types_create_tensor(exports_cosmonic_onnx_runtime_types_tensor_type_t type, onnx_runtime_impl_list_u8_t *data, onnx_runtime_impl_list_u64_t *dims, exports_cosmonic_onnx_runtime_types_own_tensor_t *ret, exports_cosmonic_onnx_runtime_types_error_t *err) {
    if (data == NULL || dims == NULL) {
        abort();
    }

    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);

    Ort::Value *ort_value = nullptr;
    void *data_buffer = nullptr;

    if (type == EXPORTS_COSMONIC_ONNX_RUNTIME_TYPES_TENSOR_TYPE_FLOAT16) {
        // printf("Creating Float16 tensor\n");
        // // Float16 - 2 bytes per element
        // size_t element_count = data->len / sizeof(Ort::Float16_t);
        // Ort::Float16_t *f16_buffer = new Ort::Float16_t[element_count];
        // memcpy(f16_buffer, data->ptr, data->len);
        // data_buffer = f16_buffer;

        // ort_value = new Ort::Value(Ort::Value::CreateTensor<Ort::Float16_t>(
        //     memory_info,
        //     f16_buffer,
        //     element_count,
        //     reinterpret_cast<int64_t*>(dims->ptr),
        //     dims->len
        // ));
        abort();
    } else {
        printf("Creating Float32 tensor\n");
        // Float32 (default) - 4 bytes per element
        size_t element_count = data->len / sizeof(float);
        float *f32_buffer = new float[element_count];
        memcpy(f32_buffer, data->ptr, data->len);
        data_buffer = f32_buffer;


        // // Debug: verify tensor was created correctly
        // auto shape = ort_value->GetTensorTypeAndShapeInfo().GetShape();
        // std::cout << "Created tensor shape: [";
        // for (auto dim : shape) std::cout << dim << ",";
        // std::cout << "]" << std::endl;

        // // Verify first few values
        // float* tensor_data = ort_value->GetTensorMutableData<float>();
        // std::cout << "Tensor first 5 values: ";
        // for (int i = 0; i < 5; i++) std::cout << tensor_data[i] << " ";
        // std::cout << std::endl;

        // // ort_value = new Ort::Value(Ort::Value::CreateTensor<float>(
        // //     memory_info,
        // //     f32_buffer,
        // //     element_count,
        // //     reinterpret_cast<int64_t*>(dims->ptr),
        // //     dims->len
        // // ));

        // Convert uint64_t dims to int64_t dims safely
        std::vector<int64_t> int64_dims(dims->len);
        for (size_t i = 0; i < dims->len; i++) {
            int64_dims[i] = static_cast<int64_t>(dims->ptr[i]);
            std::cout << "dim[" << i << "] = " << int64_dims[i] << std::endl;
        }

        // Validate dimensions match element count
        int64_t calculated_elements = 1;
        for (size_t i = 0; i < int64_dims.size(); i++) {
            std::cout << "dim[" << i << "] = " << int64_dims[i] << std::endl;
            if (int64_dims[i] <= 0) {
                std::cerr << "ERROR: Invalid dimension at index " << i << ": " << int64_dims[i] << std::endl;
                abort();
            }
            calculated_elements *= int64_dims[i];
        }

        std::cout << "Calculated elements from dims: " << calculated_elements << std::endl;
        std::cout << "Actual element count: " << element_count << std::endl;

        if (calculated_elements != element_count) {
            std::cerr << "ERROR: Dimension mismatch! dims product=" << calculated_elements
                    << " but element_count=" << element_count << std::endl;
            abort();
        }

        ort_value = new Ort::Value(Ort::Value::CreateTensor<float>(
            memory_info,
            f32_buffer,
            element_count,
            int64_dims.data(),  // Use converted dims
            int64_dims.size()
        ));
    }

    if (!ort_value->IsTensor()) {
        abort();
    }

    if (ort_value == nullptr || *ort_value == nullptr) {
        if (type == EXPORTS_COSMONIC_ONNX_RUNTIME_TYPES_TENSOR_TYPE_FLOAT16) {
            delete[] static_cast<Ort::Float16_t*>(data_buffer);
        } else {
            delete[] static_cast<float*>(data_buffer);
        }
        abort();
    }

    exports_cosmonic_onnx_runtime_types_tensor_t *tensor = new exports_cosmonic_onnx_runtime_types_tensor_t{
        .tensor = ort_value,
        .data_buffer = data_buffer
    };

    *ret = exports_cosmonic_onnx_runtime_types_tensor_new(tensor);
    return true;
}


// bool exports_cosmonic_onnx_runtime_types_create_tensor(exports_cosmonic_onnx_runtime_types_tensor_type_t type, onnx_runtime_impl_list_u8_t *data, onnx_runtime_impl_list_u64_t *dims, exports_cosmonic_onnx_runtime_types_own_tensor_t *ret, exports_cosmonic_onnx_runtime_types_error_t *err) {
//     if (data == NULL || dims == NULL) {
//         abort();
//     }

//     Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

//     size_t element_count = data->len / sizeof(float);
//     Ort::Value *ort_value = new Ort::Value(Ort::Value::CreateTensor<float>(
//         memory_info,
//         reinterpret_cast<float*>(data->ptr),
//         element_count, // TODO: is size correct?
//         reinterpret_cast<int64_t*>(dims->ptr),
//         dims->len // TODO: is size correct?
//     ));
//     if (ort_value == nullptr) {
//         abort();
//     }

//     exports_cosmonic_onnx_runtime_types_tensor_t tensor = {
//         .tensor = ort_value
//     };
//     *ret = exports_cosmonic_onnx_runtime_types_tensor_new(&tensor);
//     return true;
// }

void exports_cosmonic_onnx_runtime_types_method_tensor_get_dims(exports_cosmonic_onnx_runtime_types_borrow_tensor_t self, onnx_runtime_impl_list_u64_t *ret) {
    if (self == NULL || ret == NULL) {
        abort();
    }

    ret->len = self->tensor->GetTensorTypeAndShapeInfo().GetShape().size();
    ret->ptr = new uint64_t[ret->len];
    for (size_t i = 0; i < ret->len; i++) {
        ret->ptr[i] = self->tensor->GetTensorTypeAndShapeInfo().GetShape()[i];
    }
}

// void exports_cosmonic_onnx_runtime_types_method_tensor_get_data(exports_cosmonic_onnx_runtime_types_borrow_tensor_t self, bool *maybe_release_data, onnx_runtime_impl_list_u8_t *ret) {
//     if (self == NULL || ret == NULL) {
//         abort();
//     }

//     ret->len = self->tensor->GetTensorTypeAndShapeInfo().GetElementCount();
//     ret->ptr = new uint8_t[ret->len];
//     memcpy(ret->ptr, self->tensor->GetTensorData<uint8_t>(), ret->len);
// }

void exports_cosmonic_onnx_runtime_types_method_tensor_get_data(exports_cosmonic_onnx_runtime_types_borrow_tensor_t self, bool *maybe_release_data, onnx_runtime_impl_list_u8_t *ret) {
    if (self == NULL || ret == NULL) {
        abort();
    }

    auto type_info = self->tensor->GetTensorTypeAndShapeInfo();
    size_t element_count = type_info.GetElementCount();
    size_t element_size = 0;

    // Determine element size based on data type
    auto element_type = type_info.GetElementType();
    switch (element_type) {
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT:
            element_size = sizeof(float);  // 4 bytes
            break;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16:
            element_size = 2;  // 2 bytes
            break;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32:
            element_size = sizeof(int32_t);
            break;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64:
            element_size = sizeof(int64_t);
            break;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8:
            element_size = 1;
            break;
        default:
            element_size = sizeof(float);  // Default to float
            break;
    }

    size_t byte_count = element_count * element_size;
    ret->len = byte_count;
    ret->ptr = new uint8_t[byte_count];
    memcpy(ret->ptr, self->tensor->GetTensorData<uint8_t>(), byte_count);
}

// void exports_cosmonic_onnx_runtime_types_error_free(exports_cosmonic_onnx_runtime_types_error_t *ptr);

// void exports_cosmonic_onnx_runtime_types_option_graph_optimization_level_free(exports_cosmonic_onnx_runtime_types_option_graph_optimization_level_t *ptr);

// void exports_cosmonic_onnx_runtime_types_session_options_free(exports_cosmonic_onnx_runtime_types_session_options_t *ptr);

// void exports_cosmonic_onnx_runtime_types_option_log_severity_level_free(exports_cosmonic_onnx_runtime_types_option_log_severity_level_t *ptr);

// void onnx_runtime_impl_option_u32_free(onnx_runtime_impl_option_u32_t *ptr);

// void onnx_runtime_impl_option_bool_free(onnx_runtime_impl_option_bool_t *ptr);

// void onnx_runtime_impl_option_string_free(onnx_runtime_impl_option_string_t *ptr);

// void onnx_runtime_impl_tuple2_string_string_free(onnx_runtime_impl_tuple2_string_string_t *ptr);

// void onnx_runtime_impl_list_tuple2_string_string_free(onnx_runtime_impl_list_tuple2_string_string_t *ptr);

// void exports_cosmonic_onnx_runtime_types_run_options_free(exports_cosmonic_onnx_runtime_types_run_options_t *ptr);

// extern void exports_cosmonic_onnx_runtime_types_session_drop_own(exports_cosmonic_onnx_runtime_types_own_session_t handle);

// extern exports_cosmonic_onnx_runtime_types_own_session_t exports_cosmonic_onnx_runtime_types_session_new(exports_cosmonic_onnx_runtime_types_session_t *rep);
// extern exports_cosmonic_onnx_runtime_types_session_t* exports_cosmonic_onnx_runtime_types_session_rep(exports_cosmonic_onnx_runtime_types_own_session_t handle);
void exports_cosmonic_onnx_runtime_types_session_destructor(exports_cosmonic_onnx_runtime_types_session_t *rep) {
    if (rep == NULL) {
        abort();
    }

    delete rep->session;
    delete rep->env;
    delete rep;
}

// extern void exports_cosmonic_onnx_runtime_types_tensor_drop_own(exports_cosmonic_onnx_runtime_types_own_tensor_t handle);

// extern exports_cosmonic_onnx_runtime_types_own_tensor_t exports_cosmonic_onnx_runtime_types_tensor_new(exports_cosmonic_onnx_runtime_types_tensor_t *rep);
// extern exports_cosmonic_onnx_runtime_types_tensor_t* exports_cosmonic_onnx_runtime_types_tensor_rep(exports_cosmonic_onnx_runtime_types_own_tensor_t handle);
void exports_cosmonic_onnx_runtime_types_tensor_destructor(exports_cosmonic_onnx_runtime_types_tensor_t *rep) {
    if (rep == NULL) {
        abort();
    }

    if (rep->tensor == NULL) {
        abort();
    }

    delete rep->tensor;
    delete[] rep->data_buffer;
}

// void onnx_runtime_impl_list_u8_free(onnx_runtime_impl_list_u8_t *ptr);

// void exports_cosmonic_onnx_runtime_types_option_session_options_free(exports_cosmonic_onnx_runtime_types_option_session_options_t *ptr);

// void exports_cosmonic_onnx_runtime_types_result_own_session_error_free(exports_cosmonic_onnx_runtime_types_result_own_session_error_t *ptr);

// void exports_cosmonic_onnx_runtime_types_tuple2_string_own_tensor_free(exports_cosmonic_onnx_runtime_types_tuple2_string_own_tensor_t *ptr);

// void exports_cosmonic_onnx_runtime_types_list_tuple2_string_own_tensor_free(exports_cosmonic_onnx_runtime_types_list_tuple2_string_own_tensor_t *ptr);

// void exports_cosmonic_onnx_runtime_types_option_run_options_free(exports_cosmonic_onnx_runtime_types_option_run_options_t *ptr);

// void exports_cosmonic_onnx_runtime_types_result_list_tuple2_string_own_tensor_error_free(exports_cosmonic_onnx_runtime_types_result_list_tuple2_string_own_tensor_error_t *ptr);

// void onnx_runtime_impl_list_u64_free(onnx_runtime_impl_list_u64_t *ptr);

// void exports_cosmonic_onnx_runtime_types_result_own_tensor_error_free(exports_cosmonic_onnx_runtime_types_result_own_tensor_error_t *ptr);


// helper functions

GraphOptimizationLevel graph_optimization_level_wit_to_cpp(exports_cosmonic_onnx_runtime_types_graph_optimization_level_t graph_optimization_level) {
    switch (graph_optimization_level) {
        case EXPORTS_COSMONIC_ONNX_RUNTIME_TYPES_GRAPH_OPTIMIZATION_LEVEL_DISABLED:
            return GraphOptimizationLevel::ORT_DISABLE_ALL;
        case EXPORTS_COSMONIC_ONNX_RUNTIME_TYPES_GRAPH_OPTIMIZATION_LEVEL_BASIC:
            return GraphOptimizationLevel::ORT_ENABLE_BASIC;
        case EXPORTS_COSMONIC_ONNX_RUNTIME_TYPES_GRAPH_OPTIMIZATION_LEVEL_EXTENDED:
            return GraphOptimizationLevel::ORT_ENABLE_EXTENDED;
        case EXPORTS_COSMONIC_ONNX_RUNTIME_TYPES_GRAPH_OPTIMIZATION_LEVEL_LAYOUT:
            return GraphOptimizationLevel::ORT_ENABLE_LAYOUT;
        case EXPORTS_COSMONIC_ONNX_RUNTIME_TYPES_GRAPH_OPTIMIZATION_LEVEL_ALL:
            return GraphOptimizationLevel::ORT_ENABLE_ALL;
    }
}

onnx_runtime_impl_string_t string_cpp_to_wasi(const char* string_cpp) {
    if (!string_cpp) abort();

    onnx_runtime_impl_string_t string_wasi = {};
    string_wasi.ptr = (uint8_t*) malloc(strlen(string_cpp));
    if (string_wasi.ptr == NULL) abort();
    size_t length = strlen(string_cpp);
    memcpy(string_wasi.ptr, string_cpp, length);
    string_wasi.len = length;

    return string_wasi;
}


const char* string_wasi_to_cpp(onnx_runtime_impl_string_t string_wasi) {
    char* string_cpp = (char*) malloc(string_wasi.len + 1);
    if (string_cpp == NULL) abort();
    memcpy((char*) string_cpp, string_wasi.ptr, string_wasi.len);
    string_cpp[string_wasi.len] = '\0';
    return string_cpp;
}
