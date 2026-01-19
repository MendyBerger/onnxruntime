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
    float *data_buffer;  // Add this to track the copied data buffer
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
    session_options.AppendExecutionProvider("WebGPU", webgpu_options);
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



    exports_cosmonic_onnx_runtime_types_session_t session = {
        .session = ort_session,
        .env = env
    };
    *ret = exports_cosmonic_onnx_runtime_types_session_new(&session);
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

    for (size_t i = 0; i < output_tensors.size(); i++) {
        // Convert output name to WASI string
        ret->ptr[i].f0 = string_cpp_to_wasi(output_names_vec[i].c_str());

        // Wrap the output tensor
        Ort::Value *ort_value = new Ort::Value(std::move(output_tensors[i]));
        exports_cosmonic_onnx_runtime_types_tensor_t *tensor = new exports_cosmonic_onnx_runtime_types_tensor_t{
            .tensor = ort_value,
            .data_buffer = nullptr  // Output tensors don't need a data buffer
        };
        ret->ptr[i].f1 = exports_cosmonic_onnx_runtime_types_tensor_new(tensor);
    }



    return true;
}


bool exports_cosmonic_onnx_runtime_types_create_tensor(exports_cosmonic_onnx_runtime_types_tensor_type_t type, onnx_runtime_impl_list_u8_t *data, onnx_runtime_impl_list_u64_t *dims, exports_cosmonic_onnx_runtime_types_own_tensor_t *ret, exports_cosmonic_onnx_runtime_types_error_t *err) {
    if (data == NULL || dims == NULL) {
        abort();
    }

    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    // Calculate number of elements from bytes
    size_t element_count = data->len / sizeof(float);

    // Copy the data into a buffer that we own (so it persists)
    float *data_buffer = new float[element_count];
    if (data_buffer == nullptr) {
        abort();
    }
    memcpy(data_buffer, data->ptr, data->len);

    // Create tensor with our owned buffer
    Ort::Value *ort_value = new Ort::Value(Ort::Value::CreateTensor<float>(
        memory_info,
        data_buffer,  // Use our owned buffer
        element_count,
        reinterpret_cast<int64_t*>(dims->ptr),
        dims->len
    ));

    // TODO: remove this after testing
    if (!ort_value->IsTensor()) {
        abort_4();
    }

    if (ort_value == nullptr || *ort_value == nullptr) {
        delete[] data_buffer;  // Clean up on error
        abort();
    }

    // exports_cosmonic_onnx_runtime_types_tensor_t tensor = {
    exports_cosmonic_onnx_runtime_types_tensor_t *tensor = new exports_cosmonic_onnx_runtime_types_tensor_t{
        .tensor = ort_value,
        .data_buffer = data_buffer  // Store pointer for cleanup
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

void exports_cosmonic_onnx_runtime_types_method_tensor_get_data(exports_cosmonic_onnx_runtime_types_borrow_tensor_t self, bool *maybe_release_data, onnx_runtime_impl_list_u8_t *ret) {
    if (self == NULL || ret == NULL) {
        abort();
    }

    ret->len = self->tensor->GetTensorTypeAndShapeInfo().GetElementCount();
    ret->ptr = new uint8_t[ret->len];
    memcpy(ret->ptr, self->tensor->GetTensorData<uint8_t>(), ret->len);
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
