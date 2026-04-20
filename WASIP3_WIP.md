# WASI P3 Port: Investigation Log

## Step 1.1: Audit wasi:webgpu WIT vs ORT Requirements

**Date**: 2026-04-19
**WIT source**: `/home/colin/public/workspaces/WebAssembly/wasi-gfx/webgpu/webgpu.wit` (`wasi:webgpu@0.0.1`)
**ORT sources audited**: `webgpu_context.cc`, `buffer_manager.cc`, `shader_helper.cc`

### Verdict: WIT is sufficient. No WIT additions needed for ORT's compute use case.

### Full Coverage Table

| ORT Call | WIT Declaration | Status |
|---|---|---|
| `wgpu::CreateInstance()` | `get-gpu() -> gpu` (no Instance concept) | ✅ bridge maps CreateInstance → noop, stores gpu handle |
| `instance.RequestAdapter()` + `WaitAny` | `gpu.request-adapter()` — sync | ✅ WIT already sync/blocking |
| `adapter.HasFeature(enum)` | `gpu-adapter.features().has(string)` | ✅ bridge must map enum→string |
| `adapter.GetLimits()` | `gpu-adapter.limits() -> gpu-supported-limits` | ✅ |
| `adapter.RequestDevice()` + `WaitAny` | `gpu-adapter.request-device()` — sync result | ✅ WIT already sync/blocking |
| `Device().GetLimits()` | `gpu-device.limits()` | ✅ |
| `Device().GetFeatures()` | `gpu-device.features()` | ✅ |
| `Device().HasFeature(enum)` | `gpu-device.features().has(string)` | ✅ bridge must map enum→string |
| `Device().GetAdapterInfo()` | `gpu-device.adapter-info()` | ✅ |
| `device.PushErrorScope()` | `gpu-device.push-error-scope()` | ✅ |
| `device.PopErrorScope()` + `WaitAny` | `gpu-device.pop-error-scope()` — sync result | ✅ WIT already sync/blocking |
| `device.CreateBuffer()` | `gpu-device.create-buffer()` | ✅ |
| `device.CreateShaderModule()` | `gpu-device.create-shader-module()` | ✅ |
| `device.CreateComputePipeline()` | `gpu-device.create-compute-pipeline()` | ✅ |
| `wgpuDeviceCreateBindGroup()` | `gpu-device.create-bind-group()` | ✅ |
| `device.CreateQuerySet()` | `gpu-device.create-query-set()` | ✅ |
| `device.GetQueue()` | `gpu-device.queue()` | ✅ |
| `queue.WriteBuffer(ptr, size)` | `gpu-queue.write-buffer-with-copy(data: list<u8>)` | ✅ copy semantics — bridge wraps raw ptr into list |
| `queue.Submit()` | `gpu-queue.submit()` | ✅ |
| `wgpuBufferGetSize()` | `gpu-buffer.size()` | ✅ |
| `wgpuBufferGetUsage()` | `gpu-buffer.usage()` | ✅ |
| `wgpuBufferGetMapState()` | `gpu-buffer.map-state()` | ✅ |
| `buffer.MapAsync()` + `WaitAny` | `gpu-buffer.map-async()` — sync result | ✅ WIT already sync/blocking |
| `wgpuBufferGetMappedRange() -> void*` | `gpu-buffer.get-mapped-range-set-with-copy()` | ⚠️ copy semantics — bridge must maintain local buffer |
| `buffer.GetConstMappedRange() -> void*` | `gpu-buffer.get-mapped-range-get-with-copy()` | ⚠️ copy semantics — bridge must maintain local buffer |
| `wgpuBufferUnmap()` / `buffer.Unmap()` | `gpu-buffer.unmap()` | ✅ |
| `buffer.Destroy()` | `gpu-buffer.destroy()` | ✅ |
| `wgpuBufferRelease()` | resource drop (implicit in component model) | ✅ |
| `wgpuBindGroupRelease()` / `wgpuBindGroupLayoutRelease()` | resource drop | ✅ |
| `encoder.CopyBufferToBuffer()` | `gpu-command-encoder.copy-buffer-to-buffer()` | ✅ |
| `encoder.ResolveQuerySet()` | `gpu-command-encoder.resolve-query-set()` | ✅ |
| `encoder.Finish()` | `gpu-command-encoder.finish()` | ✅ |
| `encoder.BeginComputePass()` | `gpu-command-encoder.begin-compute-pass()` | ✅ |
| `pass.SetPipeline()` | `gpu-compute-pass-encoder.set-pipeline()` | ✅ |
| `wgpuComputePassEncoderSetBindGroup()` | `gpu-compute-pass-encoder.set-bind-group()` | ✅ |
| `pass.DispatchWorkgroups()` | `gpu-compute-pass-encoder.dispatch-workgroups()` | ✅ |
| `pass.DispatchWorkgroupsIndirect()` | `gpu-compute-pass-encoder.dispatch-workgroups-indirect()` | ✅ |
| `pass.End()` | `gpu-compute-pass-encoder.end()` | ✅ |
| `pipeline.GetBindGroupLayout(0)` | `gpu-compute-pipeline.get-bind-group-layout()` | ✅ |
| Feature: `TimestampQuery` | `gpu-feature-name.timestamp-query` | ✅ |
| Feature: `ShaderF16` | `gpu-feature-name.shader-f16` | ✅ |
| Feature: `Subgroups` | `gpu-feature-name.subgroups` | ✅ |
| Feature: `ChromiumExperimentalTimestampQueryInsidePasses` | Not in WIT | ✅ guarded `#if !defined(__wasm__)` |
| Feature: `ChromiumExperimentalSubgroupMatrix` | Not in WIT | ✅ guarded `#if !defined(__wasm__)` |
| Feature: `BufferMapExtendedUsages` | Not in WIT | ✅ guarded `#if !defined(__wasm__)` |
| Limits: `maxBindGroups`, `maxComputeWorkgroup*`, `maxStorageBuffer*`, `maxBufferSize`, `maxBindingsPerBindGroup`, `maxComputeInvocationsPerWorkgroup` | All present in `gpu-supported-limits` | ✅ |
| Adapter info: vendor, arch, device, description | `gpu-adapter-info` | ✅ |
| Subgroup size: min/max | `gpu-adapter-info.subgroup-min-size`, `subgroup-max-size` | ✅ |

### Bridge-Level Concerns (not WIT gaps, but wasi-webgpu-headers must handle)

#### 1. Mapped range pointer semantics (HIGH complexity)
ORT calls `wgpuBufferGetMappedRange() -> void*` expecting direct memory access. WIT only has copy-based ops. Bridge must maintain per-buffer client-side allocation:

- **Write path**: `GetMappedRange()` → allocate local buf, return ptr. `Unmap()` → call `get-mapped-range-set-with-copy(local_buf)`, free.
- **Read path**: `GetConstMappedRange()` → call `get-mapped-range-get-with-copy()`, cache `list<u8>`, return ptr. `Unmap()` → free cached list.

#### 2. Feature name enum→string mapping
ORT calls `adapter.HasFeature(wgpu::FeatureName::TimestampQuery)`. WIT uses `gpu-supported-features.has("timestamp-query")`. Bridge needs `WGPUFeatureName` enum → WIT string name table.

#### 3. Instance concept → `get-gpu()`
ORT calls `wgpu::CreateInstance()` then `instance.RequestAdapter()`. WIT entry point is `get-gpu() -> gpu`. Bridge: `CreateInstance` is a no-op that calls `get-gpu()` and stores the handle; `instance.RequestAdapter()` → `gpu.request-adapter()`.

#### 4. `required-limits` as string map
`gpu-device-descriptor.required-limits` is `option<record-option-gpu-size64>` (string→u64 resource). Bridge must translate `wgpu::Limits` struct fields to string keys (e.g. `"maxStorageBufferBindingSize"`) when calling `request-device`.

### Design Note: Async operations

Current WIT makes all GPU async ops synchronous from the component's perspective (`map-async`, `request-device`, `request-adapter`, `pop-error-scope` all return results directly). This is a **P2-style interface running under P3**.

Proper P3 design would use `future<T>` for these, letting the component yield during GPU waits so the host can schedule other tasks. For ORT's initial port this is fine — ORT already calls `WaitAny(UINT64_MAX)` and blocks anyway. Upgrading to `future<T>` would require WIT changes (Mendy as champion), bridge changes (future polling in C), and runtime changes (wasi-gfx-runtime async host functions), but would not change ORT's behavior since it always blocks until done.

**Tracked as future improvement (plan Step 3.5), not a blocker.**

---

## Step 1.2: Audit wasi-gfx-runtime Implementation Completeness

**Date**: 2026-04-19
**Source**: `/home/colin/public/workspaces/wasi-gfx/wasi-gfx-runtime/crates/wasi-webgpu-wasmtime/src/trait_impls.rs`

### Verdict: Well-implemented for ORT's compute path. One notable feature gap, all else ready.

### All ORT-critical operations: ✅ Implemented

| Operation | Status |
|---|---|
| `get_gpu`, `request_adapter`, `request_device` | ✅ full wgpu backend |
| `device.features`, `limits`, `adapter_info`, `queue` | ✅ |
| `device.create_buffer`, `create_shader_module`, `create_compute_pipeline` | ✅ |
| `device.create_bind_group`, `create_bind_group_layout`, `create_pipeline_layout` | ✅ |
| `device.create_command_encoder`, `create_query_set` | ✅ |
| `device.push_error_scope`, `pop_error_scope` | ✅ |
| `queue.submit`, `queue.write_buffer_with_copy` | ✅ |
| `buffer.size`, `usage`, `map_state`, `map_async`, `unmap`, `destroy` | ✅ |
| `buffer.get_mapped_range_get_with_copy`, `get_mapped_range_set_with_copy` | ✅ |
| `command_encoder.begin_compute_pass` (incl. timestamp_writes) | ✅ |
| `command_encoder.copy_buffer_to_buffer`, `resolve_query_set`, `finish` | ✅ |
| `compute_pass.set_pipeline`, `set_bind_group`, `dispatch_workgroups`, `dispatch_workgroups_indirect`, `end` | ✅ |
| `compute_pipeline.get_bind_group_layout` | ✅ |
| `gpu_supported_limits` — all limits ORT reads | ✅ |
| `adapter.features`, `adapter.limits`, `adapter.info` | ✅ |
| `adapter_info.vendor`, `device`, `subgroup_min_size`, `subgroup_max_size` | ✅ |

### Notable gap: `subgroups` feature returns `false` (line 2834)

`GpuSupportedFeatures::has("subgroups")` is commented out — always returns `false`. ORT uses this to enable `enable subgroups;` in shaders and add subgroup builtins to `main()`. With `false`, ORT skips subgroups entirely. **Acceptable for initial port**, minor perf regression on capable hardware.

`shader-f16` ✅ (line 2823), `timestamp-query` ✅ (line 2819).

### Minor `todo!()`s irrelevant to ORT

| Function | ORT impact |
|---|---|
| `max_bind_groups_plus_vertex_buffers` | None — ORT doesn't read this limit |
| `max_inter_stage_shader_variables` | None |
| `buffer.label`, `set_label` | None |
| `query_set.destroy`, `type_`, `count` | None — ORT creates but doesn't query metadata |
| `get_mapped_range_*` error path (`todo!("Throw buffer not mapped error")`) | Only triggers on caller bug (unmapped buffer read) |
| `request_device` unexpected error variant | Only on unusual device request failure |
| `adapter.is_fallback_adapter` | Not used by ORT |
| `RecordGpuPipelineConstantValue` (all `todo!()`) | ORT doesn't use pipeline constants through WIT |

### `map_async` implementation note
Uses `async fn` + `CallbackFuture` + `instance.poll_all_devices(true)`. Blocks until GPU map completes. Correct for sync-style WIT.

## Step 1.3: Audit wasi-webgpu-headers C Bridge Completeness

**Date**: 2026-04-19
**Source**: `/home/colin/public/workspaces/wasi-gfx/wasi-webgpu-headers/webgpu.c`

### Verdict: Bridge has critical gaps. 9 ORT-required C functions missing (all trivial to add — WIT+runtime already ready).

### Implemented ✅ (ORT-critical)

| C function | Notes |
|---|---|
| `wgpuCreateInstance` | → `get_gpu()` ✅ |
| `wgpuInstanceRequestAdapter` | sync callback ✅ |
| `wgpuInstanceWaitAny` | returns `Success` immediately — all ops already sync ✅ |
| `wgpuAdapterHasFeature` | enum→string conversion done ✅ |
| `wgpuAdapterRequestDevice` | sync callback, full descriptor translation incl. required_features + required_limits ✅ |
| `wgpuDeviceHasFeature` | ✅ |
| `wgpuDeviceGetQueue` | ✅ |
| `wgpuDeviceCreateBuffer` | ✅ |
| `wgpuDeviceCreateCommandEncoder` | ✅ |
| `wgpuDeviceCreateShaderModule` | WGSL only (SPIRV `todo()` — not needed) ✅ |
| `wgpuDeviceCreateComputePipeline` | incl. override constants ✅ |
| `wgpuDeviceCreateBindGroup` | buffer bindings ✅ (sampler/texture `todo()` — not in compute path) |
| `wgpuCommandEncoderBeginComputePass` | incl. `timestampWrites` ✅ |
| `wgpuCommandEncoderCopyBufferToBuffer` | ✅ |
| `wgpuCommandEncoderFinish` | ✅ |
| `wgpuComputePassEncoderSetPipeline`, `SetBindGroup`, `End` | ✅ |
| `wgpuComputePassEncoderDispatchWorkgroups` | ✅ |
| `wgpuComputePipelineGetBindGroupLayout` | ✅ |
| `wgpuQueueSubmit` | ✅ |
| `wgpuQueueWriteBuffer` | wraps `write_buffer_with_copy` ✅ |
| `wgpuBufferGetSize/Usage/MapState/MapAsync/Unmap/Release` | ✅ |
| `wgpuBufferGetMappedRange` / `GetConstMappedRange` | copy-based client-side buffer — correct ✅ |

### Critical gaps — ORT cannot run without these

All have WIT declarations + runtime implementations. Bridge wrappers just missing.

| Missing C function | ORT usage | Severity |
|---|---|---|
| `wgpuDeviceGetLimits` | `Device().GetLimits()` cached at init, used in shader validation | **CRITICAL** |
| `wgpuDeviceGetFeatures` | `Device().GetFeatures()` builds feature set at init | **CRITICAL** |
| `wgpuDeviceGetAdapterInfo` | `Device().GetAdapterInfo()` cached at init | **CRITICAL** |
| `wgpuAdapterGetLimits` | `adapter.GetLimits()` needed for `GetRequiredLimits()` | **CRITICAL** |
| `wgpuDeviceCreatePipelineLayout` | ORT creates explicit pipeline layouts | **CRITICAL** |
| `wgpuComputePassEncoderDispatchWorkgroupsIndirect` | ORT indirect dispatch for variable workloads | **HIGH** |
| `wgpuDevicePushErrorScope` / `wgpuDevicePopErrorScope` | Validation error handling | **HIGH** |
| `wgpuDeviceCreateQuerySet` | Timestamp profiling | **MEDIUM** |
| `wgpuCommandEncoderResolveQuerySet` | Timestamp readback | **MEDIUM** |

## Step 1.4: Audit dawn_wasi_webgpu_cpp C++ Shim Completeness

**Date**: 2026-04-19
**Source**: `/home/colin/public/workspaces/wasi-gfx/dawn_wasi_webgpu_cpp/webgpu_cpp.h` (commit `65c07fb`)

### Verdict: C++ shim is complete. All ORT-critical methods correctly delegate to C bridge. Not a bottleneck.

### All 9 C bridge gaps have correct C++ shim wrappers

| C++ method | Calls C bridge | Status |
|---|---|---|
| `Device::GetLimits(Limits*)` | `wgpuDeviceGetLimits` | ✅ delegates correctly |
| `Device::GetFeatures(SupportedFeatures*)` | `wgpuDeviceGetFeatures` | ✅ delegates correctly |
| `Device::GetAdapterInfo(AdapterInfo*)` | `wgpuDeviceGetAdapterInfo` | ✅ delegates correctly |
| `Adapter::GetLimits(Limits*)` | `wgpuAdapterGetLimits` | ✅ delegates correctly |
| `Device::CreatePipelineLayout(desc)` | `wgpuDeviceCreatePipelineLayout` | ✅ delegates correctly |
| `ComputePassEncoder::DispatchWorkgroupsIndirect(buf, offset)` | `wgpuComputePassEncoderDispatchWorkgroupsIndirect` | ✅ delegates correctly |
| `Device::PushErrorScope(filter)` | `wgpuDevicePushErrorScope` | ✅ delegates correctly |
| `Device::PopErrorScope(callbackMode, callback, userdata)` | `wgpuDevicePopErrorScope` | ✅ delegates correctly |
| `Device::CreateQuerySet(desc)` | `wgpuDeviceCreateQuerySet` | ✅ delegates correctly |
| `CommandEncoder::ResolveQuerySet(...)` | `wgpuCommandEncoderResolveQuerySet` | ✅ delegates correctly |

### PopErrorScope call chain (important)

ORT uses: `device_.PopErrorScope(CallbackMode::WaitAnyOnly, lambda, &status)` → `Wait(future)` → `instance_.WaitAny(f, UINT64_MAX)`

C++ shim correctly wraps lambda + userdata into `WGPUPopErrorScopeCallbackInfo` and calls `wgpuDevicePopErrorScope(device, callbackInfo)`.

When C bridge implements `wgpuDevicePopErrorScope`, it must:
1. Call WIT `pop-error-scope()` synchronously (gets `result<option<gpu-error>, ...>`)
2. Map result to `WGPUPopErrorScopeStatus` + `WGPUErrorType` + message string
3. Invoke `callbackInfo.callback(status, error_type, msg, userdata1, userdata2)` **inline** (before returning)
4. Return `WGPUFuture {.id = 0}` — `wgpuInstanceWaitAny` ignores the future ID and always returns Success

### One notable non-ORT stub

`CommandEncoder::WriteBuffer` is commented out (`abort()`). Not used by ORT — ORT uses `Queue::WriteBuffer` which is fully implemented.

### Other stubs (all non-ORT compute path)

`GetAHardwareBufferProperties`, `ImportSharedBufferMemory`, `ImportSharedFence`, `ImportSharedTextureMemory`, `InjectError` — all Android/shared-memory extensions, not in ORT's compute path, all `abort()`.

---

## Phase 1 Summary: Gap Analysis Complete

All four audit layers done. The **only actionable gaps** before ORT can run are in the **C bridge** (`webgpu.c`):

| Gap | Layer | Effort |
|---|---|---|
| 9 missing C bridge functions | `wasi-webgpu-headers/webgpu.c` | Medium — WIT+runtime+C++ shim all ready |
| `subgroups` feature disabled | `wasi-gfx-runtime/trait_impls.rs:2834` | Trivial — uncomment one line |

**WIT**: no changes needed.
**wasi-gfx-runtime**: complete except `subgroups` disabled (acceptable for initial port).
**dawn_wasi_webgpu_cpp**: complete. Pin updated `65c07fb` → `7f7cab7` (enables `static_assert` ABI checks).
**C bridge**: 9 functions need implementing (Phase 2 work).


---

## Step 2.1: WIT Additions

**Date**: 2026-04-19

### Verdict: No-op for initial port. WIT covers ORT's current API surface. No changes needed to get ORT running.

### Future work required for proper P3

The current WIT makes async GPU ops synchronous (`request-adapter`, `request-device`, `map-async`, `pop-error-scope` all return results directly). This is P2-style. Proper P3 design uses `future<T>` so the component can yield during GPU waits.

This requires WIT changes (Mendy as champion), C bridge changes, and runtime changes — but does **not** change ORT's behavior since ORT always blocks via `WaitAny(UINT64_MAX)` anyway. Tracked as plan Step 3.5. Revisit after initial port is working.

---

## Step 2.2: Host Runtime Implementation (wasi-gfx-runtime)

**Date**: 2026-04-19

### Verdict: No-op. Runtime fully implemented for ORT's compute path.

One gap exists (`subgroups` feature check disabled at `trait_impls.rs:2834`) but is acceptable for initial port — ORT simply skips subgroup shader features. Deferred, not blocking.

---

## Step 2.3: C Bridge Additions (wasi-webgpu-headers)

**Date**: 2026-04-19
**Source**: `/home/colin/public/workspaces/wasi-gfx/wasi-webgpu-headers/webgpu.c`

### Verdict: All 9 missing functions implemented. Bridge now complete for ORT's compute path.

### Implemented

| C function | Implementation notes |
|---|---|
| `wgpuAdapterGetLimits` | Calls `method_gpu_adapter_limits()`, reads all 30 fields via `limitsWasiToNative` helper |
| `wgpuDeviceGetLimits` | Same via `method_gpu_device_limits()` |
| `wgpuDeviceGetFeatures` | Enumerates all 17 known WIT feature names via `has()`, builds heap-allocated `WGPUFeatureName[]`; caller frees via `wgpuSupportedFeaturesFreeMembers` |
| `wgpuDeviceGetAdapterInfo` | Calls `method_gpu_device_adapter_info()`, copies vendor/arch/device/description strings, fills `subgroupMinSize`/`subgroupMaxSize` |
| `wgpuDeviceCreatePipelineLayout` | Translates `bindGroupLayouts` array to WIT borrow list, calls `method_gpu_device_create_pipeline_layout()` |
| `wgpuDeviceCreateQuerySet` | Maps `WGPUQueryType` → `gpu_query_type_t`, calls result-returning `method_gpu_device_create_query_set()` |
| `wgpuCommandEncoderResolveQuerySet` | Direct WIT call with borrow conversions |
| `wgpuComputePassEncoderDispatchWorkgroupsIndirect` | Direct WIT call |
| `wgpuDevicePushErrorScope` | Maps `WGPUErrorFilter` → `gpu_error_filter_t`, calls WIT |
| `wgpuDevicePopErrorScope` | Calls WIT sync, maps `gpu_error_kind_t` → `WGPUErrorType`, invokes callback inline, returns `WGPUFuture{.id=0}` |
| `wgpuSupportedFeaturesFreeMembers` | Frees heap array allocated by `wgpuDeviceGetFeatures` |

### New helper: `limitsWasiToNative`

Static helper reads all 30 `gpu_supported_limits` fields via individual WIT method calls and fills `WGPULimits*`. Inverse of existing `limitsNativeToWasi`.

### ORT context: which functions are currently active vs commented out

| Function | ORT status |
|---|---|
| `wgpuDevicePushErrorScope` / `wgpuDevicePopErrorScope` | **Active** — called every kernel dispatch via `PushErrorScope()`/`PopErrorScope()` |
| `wgpuComputePassEncoderDispatchWorkgroupsIndirect` | **Active** — used in `Flush()` for variable workloads |
| `wgpuDeviceCreateQuerySet` / `wgpuCommandEncoderResolveQuerySet` | Active path but `query_type_ == None` guards early-return (timestamp feature check commented out) |
| `wgpuDeviceGetLimits` | Commented out — `device_limits_` zero-initialized until Phase 3 uncomments it |
| `wgpuDeviceGetFeatures` | Commented out — feature set empty until Phase 3 |
| `wgpuDeviceGetAdapterInfo` | Commented out — adapter info unused until Phase 3 |
| `wgpuAdapterGetLimits` | Commented out — `GetRequiredLimits()` entirely disabled |
| `wgpuDeviceCreatePipelineLayout` | Not called by ORT — uses auto layout via `nullptr` |

---

## Step 2.4: Abseil pthread stub

**Date**: 2026-04-19
**File**: `cmake/external/abseil-cpp.cmake`

### Problem

Abseil's `CMakeLists.txt` calls `find_package(Threads REQUIRED)` unconditionally. WASI has no real pthreads — CMake's thread-finder finds nothing and aborts with a fatal error before any Abseil targets are defined.

### Fix

Inject a no-op `Threads::Threads` interface target before Abseil's CMakeLists runs:

```cmake
set(CMAKE_THREAD_LIBS_INIT "")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(Threads_FOUND TRUE)
if(NOT TARGET Threads::Threads)
  add_library(Threads::Threads INTERFACE IMPORTED)
endif()
```

This satisfies `find_package(Threads)` without linking any real thread library. Abseil then detects no thread-local support via `-mthread-model single` and falls back to `STDCPP` waiter mode (mode 4), which is safe for single-threaded WASI.

---

## Step 3.1: Update Toolchain for P3

**Date**: 2026-04-19
**Source**: `/home/colin/public/workspaces/MendyBerger/onnxruntime/cmake/wasi-sdk.cmake`

### Verdict: No change needed. Current `wasm32-wasip2` target is correct for initial port.

WASI-SDK updated to clang 22.1.0. `--target=wasm32-wasip3` is recognized as a valid triple by clang 22, but no `wasm32-wasip3` sysroot exists — only `wasm32-wasip2`. Emulated libraries (`-lwasi-emulated-signal`, `-lwasi-emulated-mman`, `-lwasi-emulated-getpid`) only ship for the `wasm32-wasip2` sysroot, so switching target triple breaks linking.

Keep `wasm32-wasip2`. wasmtime P3 host (37+) runs P2 components unchanged.

### Future: adding `future<T>` to the WIT

Full P3 async support in C is a toolchain stack question:

| Layer | Status |
|---|---|
| `clang 22` target triple `wasm32-wasip3` | ✅ recognized |
| LLVM intrinsics for P3 async wasm instructions (`task.return` etc.) | ⚠️ still landing |
| `wit-bindgen c` async generation for `future<T>` | ⚠️ immature — C backend lags Rust |
| `wasm-tools component new` P3 encoding | needs version check when the time comes |
| wasmtime P3 runtime | ✅ (37+) |

Likely path when WIT adds `future<T>`: use a **sync adapter** (WASI-SDK shim making async WIT look sync to C callers) rather than true async C code — established pattern from P2→P1 bridging. Revisit when WIT changes.

---

## Step 3.2: Re-enable Commented-Out ORT WebGPU Code

**Date**: 2026-04-19
**Files**: `webgpu_context.cc`, `shader_helper.cc`

### Verdict: All commented-out blocks re-enabled. No code changed — only comments removed.

### webgpu_context.cc changes

| Block | Location | Notes |
|---|---|---|
| `GetRequiredLimits(adapter)` call + `device_desc.requiredLimits` | device init | Re-enabled; calls `adapter.GetLimits()` → `wgpuAdapterGetLimits` now implemented |
| `Device().GetLimits(&device_limits_)` | device init cache | Re-enabled |
| `Device().GetFeatures(&supported_features)` + feature loop | device init cache | Re-enabled |
| `Device().GetAdapterInfo(&adapter_info_)` | device init cache | Re-enabled; `#if !defined(__wasm__)` guard preserved for `subgroup_matrix_configs_` nextInChain setup |
| Timestamp query type detection | device init | Re-enabled; changed `device_.HasFeature()` → `DeviceHasFeature()` (consistent with surrounding code; `DeviceHasFeature()` checks `device_features_` set populated above) |
| `GetAvailableRequiredFeatures` feature array | feature enum | Re-enabled; `HasFeature()` on adapter available (`wgpuAdapterHasFeature` exists in bridge) |
| `GetRequiredLimits` function body | limits helper | Re-enabled; calls `adapter.GetLimits()` |

### shader_helper.cc changes

| Block | Location | Notes |
|---|---|---|
| Subgroup builtins in `main()` signature (`sg_id`, `sg_size`) | header generation | Re-enabled; calls `device_.HasFeature()` → `wgpuDeviceHasFeature` implemented |
| `enable subgroups;` shader directive | shader header | Re-enabled |
| `enable chromium_experimental_subgroup_matrix;` + diagnostic | shader header | Re-enabled; `#if !defined(__wasm__)` guard preserved |

### Note on `device_.HasFeature()` vs `DeviceHasFeature()`

`shader_helper.cc` uses `device_.HasFeature()` (Dawn C++ API → `wgpuDeviceHasFeature`).
`webgpu_context.cc` uses `DeviceHasFeature()` (checks `device_features_` set).
Both work. `device_.HasFeature()` goes through the C bridge; `DeviceHasFeature()` is a local cache check. No conflict.

---

## Step 3.3: Replace Emscripten JS Interop

**Date**: 2026-04-19
**Files**: `core/framework/external_data_loader.h`, `core/framework/external_data_loader.cc`, `core/framework/tensorprotoutils.cc`, `core/providers/webgpu/external_data_loader.cc`, `core/graph/model.cc`

### Verdict: Emscripten interop replaced with POSIX file I/O. `model.cc` was already correct.

### Root cause of the commented-out code

The Emscripten path used `EM_ASM_INT` inline JavaScript (`LoadWebAssemblyExternalData`) to load external data via `Module.MountedFiles` (a browser/Node.js concept). This doesn't exist in WASI — files are accessed via pre-opened WASI directories using standard POSIX I/O.

### Changes made

| File | Change |
|---|---|
| `core/framework/external_data_loader.h` | `#if defined(__wasm__)` → `#if defined(__EMSCRIPTEN__)`: `ExternalDataLoadType` and `LoadWebAssemblyExternalData` now Emscripten-only |
| `core/framework/external_data_loader.cc` | Same guard change: `LoadWebAssemblyExternalData` (EM_ASM-based) not compiled for WASI |
| `core/framework/tensorprotoutils.cc` | Same guard change on the `__wasm__` block (~line 1168): WASI now takes the `#else` path which uses `GetFileContent` + `std::filesystem::file_size` (standard POSIX) |
| `core/providers/webgpu/external_data_loader.cc` | Rewrote with POSIX `std::ifstream`. CPU case: direct read into tensor memory. GPU case: read into CPU buffer → `WebGpuContextFactory::GetContext(0).Device().GetQueue().WriteBuffer(...)` |
| `core/graph/model.cc` | No change — `ORT_ENABLE_WEBASSEMBLY_OUTPUT_OPTIMIZED_MODEL` not set; standard `FileOpenWr` path already used |

### WASI P2 file I/O compatibility

WASI P2 libc imports (`wasm32-wasip2/libc.imports`) confirm these syscalls are available:
- `path_open` → `open()` ✅
- `fd_read` / `fd_pread` → `read()` ✅
- `fd_seek` → `lseek()` ✅
- `path_filestat_get` → `stat()` → `std::filesystem::file_size()` ✅
- No `mmap` → `MapFileIntoMemory` fails silently; `GetFileContent` falls back to `ReadFileIntoBuffer` ✅

`std::ifstream::seekg` was rejected: `std::streamoff = long = 32-bit` on wasm32. External data offsets are `FileOffsetType = int64_t`; offsets >2GB would silently overflow. `env.ReadFileIntoBuffer` takes `FileOffsetType` directly and passes it to `lseek` with `off_t` (64-bit in WASI SDK).

### GPU buffer upload detail

For WEBGPU_BUFFER tensors, `tensor.MutableDataRaw()` is a `WGPUBuffer` cast to `void*` (confirmed by `allocator.cc:37`). We construct `wgpu::Buffer gpu_buffer(static_cast<WGPUBuffer>(...))` — the C++ constructor calls `WGPUAddRef` (refcount +1), `WriteBuffer` uploads data, destructor calls `WGPURelease` (refcount −1). ORT retains ownership throughout.

`wgpuQueueWriteBuffer` is implemented in the C bridge. `wgpuDeviceGetQueue` is also implemented. No missing bridge functions.

---

## Step 3.4: Entry Point

**Date**: 2026-04-19
**Files**: `onnxruntime/wasm/simple.cpp`, `cmake/onnxruntime_webassembly.cmake`

### Verdict: WASI command model. `simple.cpp` updated; CMake linker flags fixed.

### CMake fix: removed `-Wl,--no-entry` and `-Wl,--export-all`

`-Wl,--no-entry` was in the linker flags alongside `main()` in `simple.cpp`. This flag suppresses the entry point requirement — if `crt1-command.o` isn't linked, `_start` won't be defined and `wasmtime run` will fail. Removed in favour of the default WASI SDK command model (WASI SDK wasm32-wasip2 links `crt1-command.o` by default, which provides `_start` → calls `main()`).

`-Wl,--export-all` was also removed — unnecessary for a command binary, bloats the output, and could violate the component model's closed-world assumptions by exporting internal symbols.

### `simple.cpp` changes

| Change | Reason |
|---|---|
| Accept `argv[1]` as model path | `wasmtime run --dir=. ort.wasm model.ort` — flexible testing |
| Fall back to `"simple_model.ort"` | Backward-compatible default |
| Query input/output count + names from model | Works with any model, not just the hardcoded smoke-test model |
| Replace dynamic dims (-1) with 1 | Handles models with dynamic batch size |
| Print first 10 output values generically | No longer assumes `output_shape[1]` |

### How to run (Phase 4)

```bash
# Build
WASI_SDK_PATH=/opt/wasi-sdk-24.0-x86_64-linux ./build_wasi_simple.sh

# Run — pre-open current dir for model file access
wasmtime run --dir=. build_wasi/ort.wasi.simd.wasm simple_model.ort
```

### Command vs Reactor

Current model: **WASI Command** — one `main()`, runs and exits.

If ORT needs to be called multiple times from a host (e.g., web service), switch to reactor model:
- Change `simple.cpp` to export named functions instead of `main()`
- Add `-mexec-model=reactor` to link options
- Host calls `_initialize()` once, then exported functions per request

---

## Phase 4: Build and Run

**Date**: 2026-04-20

### Build toolchain

- WASI SDK 32 (`/opt/wasi-sdk/`, clang 22)
- Node 20 via nvm (`/home/colin/.nvm/versions/node/v20.20.2/bin/node`) — required for WGSL template generator
- Build output: `build_wasi/ort-wasi-simd.wasm`
- Runtime: **graphtime** (`/mnt/raid/public/workspaces/wasi-gfx/graphtime`) — custom wasmtime with wasi:webgpu support

### Why graphtime, not wasmtime

Standard `wasmtime run` has no `wasi:webgpu` host implementation. graphtime is a thin wrapper using `wasi-gfx-runtime` crates that implement the wasi:webgpu WIT. It exposes `--dir` for preopened directories and `--` for argv passthrough (added in this session).

### graphtime changes (this session)

| Change | Reason |
|---|---|
| Added `--dir guest::host` argument | Guest needs preopened directories for file access |
| Added `-- <args>` trailing argv | Pass model path to wasm argv |
| Added `inherit_env()` to WasiCtxBuilder | `getenv("USE_WEBGPU")` otherwise returns null in guest |
| Prepend wasm file name as argv[0] | WASI command model expects argv[0] = program name |
| `std::process::exit(exit_code)` after wasm completes | Winit event loop blocks forever otherwise; process exits cleanly |

### webgpu.c fixes (this session)

All fixes in `build_wasi/_deps/wasi_webgpu_headers-src/webgpu.c` (vendored copy; upstream fix needed separately):

| Function | Fix |
|---|---|
| `wgpuAdapterGetLimits` | Implemented — was `abort()` |
| `wgpuAdapterRequestDevice` | Removed premature `drop_own` before ownership transfer (caused "unknown handle index 4") |
| `wgpuDeviceGetLimits` | Implemented — was `abort()` |
| `wgpuDeviceGetFeatures` | Implemented (empty feature list — acceptable) — was `abort()` |
| `wgpuDeviceGetAdapterInfo` | Implemented (`memset` zero — no WASI device→adapter_info path; ORT only uses vendor string for Intel detection) |
| `wgpuQueueAddRef` | Implemented (refcount increment) |
| `wgpuQueueRelease` | Implemented (refcount decrement + `drop_own` + `free`) |

### ORT source fixes (this session)

| File | Fix |
|---|---|
| `core/framework/tensorprotoutils.cc` | Changed `GetFileContent` guard from `#if !defined(__wasm__)` to `#if !defined(__EMSCRIPTEN__)` so WASI can use it |
| `core/framework/tensorprotoutils.cc` | Uncommented `ml_tensor_type` declaration (was accidentally commented in prior session; still used in `#else` branch) |

### Results

| Mode | Command | Output |
|---|---|---|
| CPU only | `graphtime ort-wasi-simd.wasm --dir ".::/path/to/ort" -- simple_model.ort` | `[2, 2, 2, 2, 2]` ✅ |
| WebGPU | `USE_WEBGPU=1 graphtime ort-wasi-simd.wasm --dir ".::/path/to/ort" -- simple_model.ort` | `[2, 2, 2, 2, 2]` ✅ |

Both exit cleanly with code 0.

### How to run

```bash
# Build
cd /mnt/raid/public/workspaces/MendyBerger/onnxruntime
./build_wasi_simple.sh

# CPU inference
cd /mnt/raid/public/workspaces/wasi-gfx/graphtime
xvfb-run -a cargo run --release -- \
  /mnt/raid/public/workspaces/MendyBerger/onnxruntime/build_wasi/ort-wasi-simd.wasm \
  --dir ".::/mnt/raid/public/workspaces/MendyBerger/onnxruntime" \
  -- simple_model.ort

# WebGPU inference
USE_WEBGPU=1 xvfb-run -a cargo run --release -- \
  /mnt/raid/public/workspaces/MendyBerger/onnxruntime/build_wasi/ort-wasi-simd.wasm \
  --dir ".::/mnt/raid/public/workspaces/MendyBerger/onnxruntime" \
  -- simple_model.ort
```

### xvfb-run note

graphtime uses winit which requires a display (even for headless GPU compute). `xvfb-run -a` provides a virtual framebuffer. On a system with a real display or a GPU server with display configured, this is not needed.

### Known remaining gaps

| Issue | Impact | Fix |
|---|---|---|
| `wgpuDeviceGetFeatures` returns empty list | ORT skips subgroup shaders, f16, timestamp profiling | Implement feature enumeration via WASI `has()` for each feature name |
| `wgpuDeviceGetAdapterInfo` returns zeroed struct | ORT skips Intel-specific transpose optimization | Implement via `method_gpu_adapter_get_info()` — but device→adapter path unclear in WASI WIT |
| graphtime process-exit approach | Winit event loop never gets clean shutdown | Proper fix: signal event loop via channel; `std::process::exit` is acceptable for CLI use |
| webgpu.c fixes not upstreamed | Vendored copy only | Open PRs to wasi-webgpu-headers upstream |
