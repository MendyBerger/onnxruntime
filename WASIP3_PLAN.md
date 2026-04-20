# Plan: ONNXRuntime on WASI P3 with wasi:webgpu

## Context

ONNXRuntime's WebGPU provider was previously ported to WASI P2 in the `wasi-main` branch. CPU-only P1 worked, but P2 with WebGPU failed because the Component Model's closed-world validation requires the host to implement **100% of declared WIT imports** at instantiation time — the wasi-gfx-runtime didn't have complete coverage.

The goal now is to make this work on **WASI P3** with **WebGPU**, not CPU, which is available in wasmtime 37+ (wasi-gfx-runtime already tracks wasmtime 43). P3's native async (`stream<T>`, `future<T>`) is a better fit for WebGPU's async operations (buffer mapping, device creation). The closed-world requirement is **unchanged in P3** — every declared import must still be satisfied.

**Key ecosystem facts:**
- **wasi:webgpu WIT**: ~1085 lines, 38 resource types, Phase 2 proposal. Champions: Mendy Berger, Sean Isom
- **wasi-gfx-runtime**: Rust host backed by wgpu (v29), wasmtime 43. Has working examples (compute, rendering)
- **wasi-webgpu-headers**: C bindings mapping webgpu-native API → wasi:webgpu WIT imports
- **dawn_wasi_webgpu_cpp**: C++ wrapper mimicking Dawn API over the C bindings

---

## The Stack (bottom to top)

```
┌─────────────────────────────────────┐
│  ONNXRuntime WebGPU Provider        │  ← ORT source (C++)
│  (uses wgpu:: C++ API)              │
├─────────────────────────────────────┤
│  dawn_wasi_webgpu_cpp               │  ← C++ shim (webgpu_cpp.h)
│  (Dawn C++ API → webgpu-native C)   │
├─────────────────────────────────────┤
│  wasi-webgpu-headers                │  ← C bridge (webgpu.h → WIT imports)
│  (webgpu-native C → wasi:webgpu)    │
├─────────────────────────────────────┤
│  Component Model boundary           │  ← WASI P3 component
│  (WIT-declared imports)             │
├─────────────────────────────────────┤
│  wasi-gfx-runtime                   │  ← Host (Rust, backed by wgpu)
│  (wasmtime + wasi:webgpu impl)      │
└─────────────────────────────────────┘
```

Each layer can have gaps. The plan audits each layer bottom-up.

---

## Phase 1: Audit the gaps at each layer

### Step 1.1: Audit wasi:webgpu WIT vs ORT requirements

ORT's WebGPU provider uses a specific compute-focused subset of WebGPU. Compare ORT's API surface against the WIT to find anything ORT needs that isn't declared.

**ORT requires (from exploration):**
- Instance creation, adapter request, device request
- Buffer: create, map/unmap, getSize, getUsage, getMapState, getMappedRange, destroy, release
- Shader module creation (WGSL)
- Compute pipeline: create, getBindGroupLayout
- Bind group: create, release
- Command encoder: create, beginComputePass, copyBufferToBuffer, finish
- Compute pass: setPipeline, setBindGroup, dispatchWorkgroups, dispatchWorkgroupsIndirect, end
- Queue: submit, writeBuffer
- Device limits query (GetLimits)
- Device feature query (GetFeatures, HasFeature)
- Adapter info query (GetAdapterInfo)
- Error handling (pushErrorScope, popErrorScope, uncaptured error callback)
- QuerySet (timestamp queries — optional but desired)

**Action**: Clone wasi-gfx repo, read `webgpu.wit`, check each item above. The WIT has 38 resource types so most should be there. Flag any gaps.

**Files:**
- `https://github.com/WebAssembly/wasi-gfx` — `wit/webgpu.wit`

### Step 1.2: Audit wasi-gfx-runtime implementation completeness

The closed-world failure means: even if the WIT declares a function, the host must implement it. wasi-gfx-runtime uses wgpu and has working examples, but likely has stubs or unimplemented functions.

**Action**: Clone wasi-gfx-runtime, search for `todo!()`, `unimplemented!()`, `unreachable!()`, or empty function bodies in the `wasi-webgpu-wasmtime` crate. Map these against what ORT needs.

**Files:**
- `https://github.com/wasi-gfx/wasi-gfx-runtime` — `crates/wasi-webgpu-wasmtime/src/`

### Step 1.3: Audit wasi-webgpu-headers C bridge completeness

Check which webgpu-native C API functions are actually implemented vs stubbed. Some may just be declared but not wired to WIT imports.

**Action**: Read the generated `webgpu.c` / `imports.c` from wasi-webgpu-headers. Check that every C function ORT calls (from the C API list in the exploration) has a real implementation that dispatches to a WIT import.

**Files:**
- `https://github.com/MendyBerger/wasi-webgpu-headers`

### Step 1.4: Audit dawn_wasi_webgpu_cpp C++ shim completeness

Check that the C++ shim covers all `wgpu::` types and methods ORT uses, especially the ones that were commented out in the previous attempt (limits, features, adapter info).

**Action**: Read `webgpu_cpp.h` from dawn_wasi_webgpu_cpp. Check whether `Device::GetLimits()`, `Adapter::GetLimits()`, `Device::GetFeatures()`, `Adapter::HasFeature()`, `Device::GetAdapterInfo()` are present or stubbed.

**Files:**
- `https://github.com/MendyBerger/dawn_wasi_webgpu_cpp`

---

## Phase 2: Fill the gaps (across repos)

Based on audit findings, work will be needed in some or all of:

### Step 2.1: WIT additions (if needed)

If any ORT-required functionality is missing from the WIT, propose additions. Given Mendy is a champion, this is feasible. Likely candidates:
- Device limits/features queries (may already be there as `gpu-supported-limits`, `gpu-supported-features`)
- Adapter info

### Step 2.2: Host runtime implementation (wasi-gfx-runtime)

For any WIT-declared function that's unimplemented in the runtime, add the wgpu-backed implementation. Priority order:
1. Device/adapter creation and capability queries
2. Buffer operations (create, map, copy)
3. Compute pipeline creation and dispatch
4. Bind group management
5. Error handling
6. Timestamp queries (nice-to-have)

### Step 2.3: C bridge additions (wasi-webgpu-headers)

Regenerate or update the C bindings if the WIT changed. Ensure all C functions ORT calls are wired through.

### Step 2.4: C++ shim additions (dawn_wasi_webgpu_cpp)

Add any missing C++ wrapper methods that ORT needs, especially:
- `Device::GetLimits()` / `Adapter::GetLimits()`
- `Device::GetFeatures()` / `Adapter::HasFeature()`
- `Device::GetAdapterInfo()`

---

## Phase 3: ORT source changes

### Step 3.1: Update toolchain for P3

- Update `cmake/wasi-sdk.cmake` to target `wasm32-wasip3` (or whatever the P3 target triple is — may still be `wasm32-wasip2` with P3 features enabled via wasmtime flags)
- Verify WASI-SDK version supports P3 component output

### Step 3.2: Re-enable commented-out WebGPU code

Once the WIT + runtime + shim layers support the full API surface, un-comment the disabled code in:
- `webgpu_context.cc`: device limits, feature queries, adapter info, timestamp queries
- `shader_helper.cc`: subgroup/ShaderF16 feature detection
- `buffer_manager.cc`: BufferMapExtendedUsages feature check

### Step 3.3: Replace Emscripten JS interop

Replace commented-out `EM_ASM` code with WASI-native alternatives:
- `external_data_loader.cc`: use POSIX file I/O (WASI filesystem) instead of Emscripten Module.MountedFiles
- `tensorprotoutils.cc`: same — load external data via filesystem
- `model.cc`: serialize model via filesystem write instead of JS blob

### Step 3.4: Entry point

Replace `wasm/simple.cpp` test harness with a proper entry point. Options:
- Keep `simple.cpp` as a demo/test component
- Build a proper `wasm/api.cc` equivalent that exports a usable interface for host applications

### Step 3.5: Async operations (P3 native async)

P3's `future<T>` and `stream<T>` could improve:
- `Instance::RequestAdapter()` — currently uses callback + WaitAny
- `Adapter::RequestDevice()` — same
- `Buffer::MapAsync()` — currently callback-based
- `Device::PopErrorScope()` — callback-based

This is optional for initial functionality but is the right long-term design for P3.

---

## Phase 4: Build and test

### Step 4.1: Build the component

```bash
cmake cmake/ \
  -DCMAKE_TOOLCHAIN_FILE=cmake/wasi-sdk.cmake \
  -Donnxruntime_USE_WEBGPU=ON \
  -Donnxruntime_ENABLE_WEBASSEMBLY_SIMD=ON \
  -Donnxruntime_DISABLE_EXCEPTIONS=ON \
  -Donnxruntime_DISABLE_RTTI=ON
cmake --build . --target onnxruntime_webassembly
```

### Step 4.2: Run with wasi-gfx-runtime

```bash
# Using graphtime or wasi-gfx-runtime
graphtime run --dir=. ort-wasi-webgpu.wasm
```

### Step 4.3: Verify end-to-end

1. Load a simple model (`simple_model.ort`)
2. Confirm WebGPU EP is registered and device is created
3. Run inference and verify output correctness
4. Test with SqueezeNet (`squeezenet.ort`) for a real workload

---

## Verification

- [ ] Component instantiates without import validation errors (the P2 blocker)
- [ ] Device limits are queried from the real adapter (not hardcoded)
- [ ] Compute pipeline creation succeeds
- [ ] Buffer map/unmap round-trips data correctly
- [ ] Simple model inference produces correct output
- [ ] SqueezeNet inference produces correct output

---

## Critical Path

The **gating question** is Step 1.2: how complete is wasi-gfx-runtime's implementation? If it has large gaps, filling those in Rust is the bulk of the work. If it's mostly complete (which is plausible given it has working compute examples), the ORT-side changes are mechanical.

## Repos that will need changes

| Repo | Owner | What changes |
|------|-------|-------------|
| `WebAssembly/wasi-gfx` | Mendy (champion) | WIT additions if needed |
| `wasi-gfx/wasi-gfx-runtime` | wasi-gfx org | Host implementation gaps |
| `MendyBerger/wasi-webgpu-headers` | Mendy | Regenerate C bindings if WIT changes |
| `MendyBerger/dawn_wasi_webgpu_cpp` | Mendy | Add missing C++ wrapper methods |
| `MendyBerger/onnxruntime` (this repo) | This fork | ORT source changes (Phase 3) |
