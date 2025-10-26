# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# WASI-SDK Configuration
message(STATUS "Configuring ONNXRuntime for WASI-SDK")

function(bundle_static_library bundled_target_name)
  function(recursively_collect_dependencies input_target)
    set(input_link_libraries LINK_LIBRARIES)
    get_target_property(input_type ${input_target} TYPE)
    if (${input_type} STREQUAL "INTERFACE_LIBRARY")
      set(input_link_libraries INTERFACE_LINK_LIBRARIES)
    endif()
    get_target_property(public_dependencies ${input_target} ${input_link_libraries})
    foreach(dependency IN LISTS public_dependencies)
      if(TARGET ${dependency})
        get_target_property(alias ${dependency} ALIASED_TARGET)
        if (TARGET ${alias})
          set(dependency ${alias})
        endif()
        get_target_property(type ${dependency} TYPE)
        if (${type} STREQUAL "STATIC_LIBRARY")
          list(APPEND static_libs ${dependency})
        endif()

        get_property(library_already_added GLOBAL PROPERTY ${target_name}_static_bundle_${dependency})
        if (NOT library_already_added)
          set_property(GLOBAL PROPERTY ${target_name}_static_bundle_${dependency} ON)
          recursively_collect_dependencies(${dependency})
        endif()
      endif()
    endforeach()
    set(static_libs ${static_libs} PARENT_SCOPE)
  endfunction()

  foreach(target_name IN ITEMS ${ARGN})
    list(APPEND static_libs ${target_name})
    recursively_collect_dependencies(${target_name})
  endforeach()

  list(REMOVE_DUPLICATES static_libs)

  set(bundled_target_full_name
    ${CMAKE_BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}${bundled_target_name}${CMAKE_STATIC_LIBRARY_SUFFIX})

  file(WRITE ${CMAKE_BINARY_DIR}/${bundled_target_name}.ar.in
    "CREATE ${bundled_target_full_name}\n" )

  foreach(target IN LISTS static_libs)
    file(APPEND ${CMAKE_BINARY_DIR}/${bundled_target_name}.ar.in
      "ADDLIB $<TARGET_FILE:${target}>\n")
  endforeach()

  file(APPEND ${CMAKE_BINARY_DIR}/${bundled_target_name}.ar.in "SAVE\n")
  file(APPEND ${CMAKE_BINARY_DIR}/${bundled_target_name}.ar.in "END\n")

  file(GENERATE
    OUTPUT ${CMAKE_BINARY_DIR}/${bundled_target_name}.ar
    INPUT ${CMAKE_BINARY_DIR}/${bundled_target_name}.ar.in)

  set(ar_tool ${CMAKE_AR})
  if (CMAKE_INTERPROCEDURAL_OPTIMIZATION)
    set(ar_tool ${CMAKE_CXX_COMPILER_AR})
  endif()

  add_custom_command(
    COMMAND ${ar_tool} -M < ${CMAKE_BINARY_DIR}/${bundled_target_name}.ar
    OUTPUT ${bundled_target_full_name}
    COMMENT "Bundling ${bundled_target_name}"
    VERBATIM)

  add_custom_target(bundling_target ALL DEPENDS ${bundled_target_full_name})
  foreach(target_name IN ITEMS ${ARGN})
    add_dependencies(bundling_target ${target_name})
  endforeach()

  add_library(${bundled_target_name} STATIC IMPORTED GLOBAL)
  set_target_properties(${bundled_target_name}
    PROPERTIES
      IMPORTED_LOCATION ${bundled_target_full_name})
  foreach(target_name IN ITEMS ${ARGN})
    set_property(TARGET ${bundled_target_name} APPEND
      PROPERTY INTERFACE_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:${target_name},INTERFACE_INCLUDE_DIRECTORIES>)
    set_property(TARGET ${bundled_target_name} APPEND
      PROPERTY INTERFACE_COMPILE_DEFINITIONS $<TARGET_PROPERTY:${target_name},INTERFACE_COMPILE_DEFINITIONS>)
  endforeach()
  add_dependencies(${bundled_target_name} bundling_target)
endfunction()

# WASI doesn't support JSEP or WebGPU directly
if (onnxruntime_USE_JSEP)
  message(WARNING "JSEP is not supported with WASI-SDK, disabling.")
  set(onnxruntime_USE_JSEP OFF)
endif()

if (onnxruntime_USE_WEBGPU)
  message(WARNING "WebGPU is not supported with WASI-SDK, disabling.")
  set(onnxruntime_USE_WEBGPU OFF)
endif()

# WASI doesn't support threads in the traditional sense
if (onnxruntime_ENABLE_WEBASSEMBLY_THREADS)
  message(WARNING "Traditional threading is not supported with WASI-SDK, disabling.")
  set(onnxruntime_ENABLE_WEBASSEMBLY_THREADS OFF)
endif()

add_compile_definitions(
  BUILD_MLAS_NO_ONNXRUNTIME
  __wasi__
)

# Override re2 compiler options to remove -pthread (not supported in WASI)
set_property(TARGET re2 PROPERTY COMPILE_OPTIONS )

if (NOT onnxruntime_USE_VCPKG)
  target_compile_options(onnx PRIVATE -Wno-unused-parameter -Wno-unused-variable)
endif()

if (onnxruntime_BUILD_WEBASSEMBLY_STATIC_LIB)
    bundle_static_library(onnxruntime_webassembly
      ${PROTOBUF_LIB}
      onnx
      onnx_proto
      onnxruntime_common
      onnxruntime_lora
      onnxruntime_flatbuffers
      onnxruntime_framework
      onnxruntime_graph
      onnxruntime_mlas
      onnxruntime_optimizer
      onnxruntime_providers
      ${PROVIDERS_JS}
      ${PROVIDERS_XNNPACK}
      ${PROVIDERS_WEBNN}
      ${PROVIDERS_WEBGPU}
      onnxruntime_session
      onnxruntime_util
      re2::re2
    )

    if (onnxruntime_ENABLE_TRAINING)
      bundle_static_library(onnxruntime_webassembly tensorboard)
    endif()

    if (onnxruntime_BUILD_UNIT_TESTS)
      file(GLOB_RECURSE onnxruntime_webassembly_test_src CONFIGURE_DEPENDS
        "${ONNXRUNTIME_ROOT}/test/wasm/test_main.cc"
        "${ONNXRUNTIME_ROOT}/test/wasm/test_inference.cc"
      )

      source_group(TREE ${REPO_ROOT} FILES ${onnxruntime_webassembly_test_src})

      add_executable(onnxruntime_webassembly_test
        ${onnxruntime_webassembly_test_src}
      )

      # WASI-specific link options
      target_link_options(onnxruntime_webassembly_test PRIVATE
        -Wl,--allow-undefined
        -Wl,--export-all
      )

      target_link_libraries(onnxruntime_webassembly_test PUBLIC
        onnxruntime_webassembly
        GTest::gtest
      )

      # Note: Tests need a WASI runtime like wasmtime or wasmer to run
      add_test(NAME onnxruntime_webassembly_test
        COMMAND wasmtime run --dir=. $<TARGET_FILE:onnxruntime_webassembly_test>
        WORKING_DIRECTORY $<TARGET_FILE_DIR:onnxruntime_webassembly_test>
      )
    endif()
else()
  file(GLOB_RECURSE onnxruntime_webassembly_src CONFIGURE_DEPENDS
    "${ONNXRUNTIME_ROOT}/wasm/api.cc"
  )

  source_group(TREE ${REPO_ROOT} FILES ${onnxruntime_webassembly_src})

  add_executable(onnxruntime_webassembly
    ${onnxruntime_webassembly_src}
  )

  # WASI supports exceptions, enable them
  if (onnxruntime_ENABLE_WEBASSEMBLY_API_EXCEPTION_CATCHING)
    message(STATUS "Exception catching enabled for WASI build")
    # WASI-SDK doesn't need special flags for exception catching
    # Exceptions are supported by default with proper unwinding
  endif()

  target_link_libraries(onnxruntime_webassembly PRIVATE
    ${PROTOBUF_LIB}
    onnx
    onnx_proto
    onnxruntime_common
    onnxruntime_lora
    onnxruntime_flatbuffers
    onnxruntime_framework
    onnxruntime_graph
    onnxruntime_mlas
    onnxruntime_optimizer
    onnxruntime_providers
    ${PROVIDERS_XNNPACK}
    onnxruntime_session
    onnxruntime_util
    re2::re2
  )
  # WASI-specific link options
  if (onnxruntime_USE_XNNPACK)
    target_link_libraries(onnxruntime_webassembly PRIVATE XNNPACK)
  endif()

  if (onnxruntime_ENABLE_TRAINING)
    target_link_libraries(onnxruntime_webassembly PRIVATE tensorboard)
  endif()

  # WASI-SDK linker options
  target_link_options(onnxruntime_webassembly PRIVATE
    -Wl,--allow-undefined
    -Wl,--export-all
    -Wl,--no-entry
    -Wl,--stack-first
    -Wl,-z,stack-size=1048576  # 1MB stack
  )

  # Memory configuration for WASI
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_options(onnxruntime_webassembly PRIVATE
      -Wl,--initial-memory=67108864  # 64MB initial
      -Wl,--max-memory=2147483648    # 2GB max
    )
    target_compile_options(onnxruntime_webassembly PRIVATE -g)
  else()
    target_link_options(onnxruntime_webassembly PRIVATE
      -Wl,--initial-memory=16777216  # 16MB initial
      -Wl,--max-memory=4294967296    # 4GB max
    )
    # Add optimization flags for release
    target_compile_options(onnxruntime_webassembly PRIVATE -O3)
    target_link_options(onnxruntime_webassembly PRIVATE -O3)
  endif()

  # RTTI configuration
  if (onnxruntime_DISABLE_RTTI)
    target_compile_options(onnxruntime_webassembly PRIVATE -fno-rtti)
  endif()

  # Profiling support
  if (onnxruntime_ENABLE_WEBASSEMBLY_PROFILING)
    message(STATUS "Profiling enabled for WASI build")
    target_compile_options(onnxruntime_webassembly PRIVATE -g)
  endif()

  # Build target name for WASI
  set(target_name_list ort)

  if (onnxruntime_ENABLE_TRAINING_APIS)
    list(APPEND target_name_list "training")
  endif()

  list(APPEND target_name_list "wasi")

  if (onnxruntime_ENABLE_WEBASSEMBLY_RELAXED_SIMD)
    list(APPEND target_name_list "relaxedsimd")
    target_compile_options(onnxruntime_webassembly PRIVATE -mrelaxed-simd)
  elseif (onnxruntime_ENABLE_WEBASSEMBLY_SIMD)
    list(APPEND target_name_list "simd")
    target_compile_options(onnxruntime_webassembly PRIVATE -msimd128)
  endif()

  list(JOIN target_name_list "-" target_name)

  # Set output name and extension
  set_target_properties(onnxruntime_webassembly PROPERTIES
    OUTPUT_NAME ${target_name}
    SUFFIX ".wasm"
  )

  message(STATUS "Building WASI target: ${target_name}.wasm")
endif()
