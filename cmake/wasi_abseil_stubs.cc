// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Stub implementations of Abseil synchronization primitives for WASI
// WASI Preview 2 component model doesn't support threading, so we provide no-op stubs

#ifdef __wasi__

#include <cstdint>
#include <cstdlib>

extern "C" {

// Abseil per-thread semaphore stubs (no-op for single-threaded WASI)
// Signature: (void*) -> void
void AbslInternalPerThreadSemPost_lts_20250512(void* /*arg*/) {
  // No-op: WASI is single-threaded
}

// Signature: (int64_t) -> int32_t (returns 0 for success)
int32_t AbslInternalPerThreadSemWait_lts_20250512(int64_t /*kernel_timeout*/) {
  // No-op: WASI is single-threaded, always succeeds immediately
  return 0;
}

// Signature: (void*) -> void
void AbslInternalPerThreadSemPoke_lts_20250512(void* /*arg*/) {
  // No-op: WASI is single-threaded
}

} // extern "C"

// Abseil LowLevelAlloc stubs (use standard allocator instead)
// These are strong symbols that will override weak symbols from libabsl_synchronization.a
namespace absl {
namespace lts_20250512 {
namespace base_internal {

// Forward declarations matching Abseil's internal structure
class LowLevelAlloc {
 public:
  struct Arena;

  // Allocate memory - just use malloc for WASI
  static void* Alloc(size_t request) __attribute__((visibility("default"))) {
    return malloc(request);
  }

  static void* AllocWithArena(size_t request, Arena* /*arena*/) __attribute__((visibility("default"))) {
    return malloc(request);
  }

  // Free memory - just use free for WASI
  static void Free(void* s) __attribute__((visibility("default"))) {
    free(s);
  }

  // Get default arena - return dummy
  static Arena* DefaultArena() __attribute__((visibility("default"))) {
    static Arena default_arena;
    return &default_arena;
  }

  // Create new arena - return dummy
  static Arena* NewArena(int32_t /*flags*/) __attribute__((visibility("default"))) {
    static Arena arena;
    return &arena;
  }

  // Delete arena - no-op for WASI
  static bool DeleteArena(Arena* /*arena*/) __attribute__((visibility("default"))) {
    return true;
  }

  struct Arena {
    // Empty struct for WASI - just needs to exist
  };
};

} // namespace base_internal
} // namespace lts_20250512
} // namespace absl

// Also provide explicit extern "C" versions with exact mangled names
// to ensure the symbols are available exactly as the linker expects
extern "C" {

// _ZN4absl12lts_2025051213base_internal13LowLevelAlloc4FreeEPv
// This is the mangled name for: absl::lts_20250512::base_internal::LowLevelAlloc::Free(void*)
void _ZN4absl12lts_2025051213base_internal13LowLevelAlloc4FreeEPv(void* ptr) __attribute__((visibility("default"), used)) {
    free(ptr);
}

// _ZN4absl12lts_2025051213base_internal13LowLevelAlloc5AllocEm
// This is the mangled name for: absl::lts_20250512::base_internal::LowLevelAlloc::Alloc(size_t)
void* _ZN4absl12lts_2025051213base_internal13LowLevelAlloc5AllocEm(size_t size) __attribute__((visibility("default"), used)) {
    return malloc(size);
}

// _ZN4absl12lts_2025051224synchronization_internal20CreateThreadIdentityEv
// This is: absl::lts_20250512::synchronization_internal::CreateThreadIdentity()
// Returns a dummy thread identity pointer for single-threaded WASI
void* _ZN4absl12lts_2025051224synchronization_internal20CreateThreadIdentityEv() __attribute__((visibility("default"), used)) {
    static char dummy_identity;
    return &dummy_identity;
}

// _ZN4absl12lts_2025051224synchronization_internal29GetOrCreateCurrentThreadIdentityEv
// This is: absl::lts_20250512::synchronization_internal::GetOrCreateCurrentThreadIdentity()
void* _ZN4absl12lts_2025051224synchronization_internal29GetOrCreateCurrentThreadIdentityEv() __attribute__((visibility("default"), used)) {
    static char dummy_identity;
    return &dummy_identity;
}

// _ZN4absl12lts_2025051224synchronization_internal24GetCurrentThreadIdentityIfPresentEv
// This is: absl::lts_20250512::synchronization_internal::GetCurrentThreadIdentityIfPresent()
void* _ZN4absl12lts_2025051224synchronization_internal24GetCurrentThreadIdentityIfPresentEv() __attribute__((visibility("default"), used)) {
    static char dummy_identity;
    return &dummy_identity;
}

// GraphCycles stubs for WASI - no-op implementations for single-threaded environment
// _ZN4absl12lts_2025051224synchronization_internal11GraphCycles10RemoveNodeEPv
// This is: absl::lts_20250512::synchronization_internal::GraphCycles::RemoveNode(void*)
// Member function, takes 'this' pointer as first argument
void _ZN4absl12lts_2025051224synchronization_internal11GraphCycles10RemoveNodeEPv(void* /*this_ptr*/, void* /*node*/) __attribute__((visibility("default"), used)) {
    // No-op: Single-threaded WASI doesn't need cycle detection
}

// _ZN4absl12lts_2025051224synchronization_internal11GraphCycles5GetIdEPv
// This is: absl::lts_20250512::synchronization_internal::GraphCycles::GetId(void*)
// Member function, takes 'this' pointer as first argument
int32_t _ZN4absl12lts_2025051224synchronization_internal11GraphCycles5GetIdEPv(void* /*this_ptr*/, void* /*node*/) __attribute__((visibility("default"), used)) {
    // Return a dummy ID for single-threaded WASI
    return 0;
}

// _ZN4absl12lts_2025051224synchronization_internal11GraphCycles3PtrENS1_7GraphIdE
// This is: absl::lts_20250512::synchronization_internal::GraphCycles::Ptr(GraphId)
// Member function, takes 'this' pointer and returns a pointer to the node
void* _ZN4absl12lts_2025051224synchronization_internal11GraphCycles3PtrENS1_7GraphIdE(void* /*this_ptr*/, int32_t /*graph_id*/) __attribute__((visibility("default"), used)) {
    // Return a dummy pointer for single-threaded WASI
    static char dummy_node;
    return &dummy_node;
}

// _ZN4absl12lts_2025051224synchronization_internal11GraphCycles10InsertEdgeENS1_7GraphIdES3_
// This is: absl::lts_20250512::synchronization_internal::GraphCycles::InsertEdge(GraphId, GraphId)
// Member function, returns bool indicating if edge was inserted (true = success, false = would create cycle)
bool _ZN4absl12lts_2025051224synchronization_internal11GraphCycles10InsertEdgeENS1_7GraphIdES3_(void* /*this_ptr*/, int32_t /*source_id*/, int32_t /*dest_id*/) __attribute__((visibility("default"), used)) {
    // Always return true - no cycles in single-threaded WASI
    return true;
}

// _ZNK4absl12lts_2025051224synchronization_internal11GraphCycles8FindPathENS1_7GraphIdES3_iPS3_
// This is: absl::lts_20250512::synchronization_internal::GraphCycles::FindPath(GraphId, GraphId, int, GraphId*) const
// Const member function (note the 'K' in mangled name), returns path length (0 if no path found)
int32_t _ZNK4absl12lts_2025051224synchronization_internal11GraphCycles8FindPathENS1_7GraphIdES3_iPS3_(
    const void* /*this_ptr*/, int32_t /*source_id*/, int32_t /*dest_id*/, int32_t /*max_path_len*/, int32_t* /*path*/) __attribute__((visibility("default"), used)) {
    // Return 0 - no path (no cycles in single-threaded WASI)
    return 0;
}

// _ZN4absl12lts_2025051224synchronization_internal11GraphCycles13GetStackTraceENS1_7GraphIdEPPPv
// This is: absl::lts_20250512::synchronization_internal::GraphCycles::GetStackTrace(GraphId, void***)
// Member function, returns the depth of the stack trace
int32_t _ZN4absl12lts_2025051224synchronization_internal11GraphCycles13GetStackTraceENS1_7GraphIdEPPPv(
    void* /*this_ptr*/, int32_t /*graph_id*/, void*** /*stack_trace*/) __attribute__((visibility("default"), used)) {
    // Return 0 - no stack trace for single-threaded WASI
    return 0;
}

// _ZN4absl12lts_2025051224synchronization_internal11GraphCycles16UpdateStackTraceENS1_7GraphIdEiPFiPPviE
// This is: absl::lts_20250512::synchronization_internal::GraphCycles::UpdateStackTrace(GraphId, int, int(*)(void**, int))
void _ZN4absl12lts_2025051224synchronization_internal11GraphCycles16UpdateStackTraceENS1_7GraphIdEiPFiPPviE(
    void* /*this_ptr*/, int32_t /*graph_id*/, int32_t /*priority*/, void* /*stack_trace_fn*/, int32_t /*max_depth*/) __attribute__((visibility("default"), used)) {
    // No-op: Stack trace tracking not needed for single-threaded WASI
}

// _ZN4absl12lts_2025051224synchronization_internal11GraphCyclesC1Ev
// This is: absl::lts_20250512::synchronization_internal::GraphCycles::GraphCycles()
void _ZN4absl12lts_2025051224synchronization_internal11GraphCyclesC1Ev(void* /*this_ptr*/) __attribute__((visibility("default"), used)) {
    // No-op constructor
}

// _ZN4absl12lts_2025051224synchronization_internal11GraphCyclesD1Ev
// This is: absl::lts_20250512::synchronization_internal::GraphCycles::~GraphCycles()
void _ZN4absl12lts_2025051224synchronization_internal11GraphCyclesD1Ev(void* /*this_ptr*/) __attribute__((visibility("default"), used)) {
    // No-op destructor
}

} // extern "C"

#endif // __wasi__
