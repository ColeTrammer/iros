#pragma once

#include <di/prelude.h>
#include <iris/core/config.h>
#include <iris/core/scheduler.h>
#include <iris/core/task.h>
#include <iris/core/task_namespace.h>
#include <iris/core/unit_test.h>
#include <iris/mm/address_space.h>

// clang-format off
#include IRIS_ARCH_INCLUDE(hw/processor_info.h)
// clang-format on

namespace iris {
struct GlobalState {
    GlobalState() {}
    ~GlobalState() {}

    /// @name Readonly fields
    /// Read-only after after kernel initialization. Ideally would be marked const.
    /// @{
    mm::PhysicalAddress max_physical_address { 0 };
    mm::VirtualAddress virtual_to_physical_offset { 0 };
    mm::VirtualAddress heap_start { 0 };
    di::Span<di::Byte const> initrd;
    ProcessorInfo processor_info;
    test::TestManager unit_test_manager;
    arch::FpuState initial_fpu_state;
    /// @}

    /// @name Mutable fields
    /// Mutable global state. These fields have internal synchronization, and so are
    /// safe to access concurrently. Typically, calling code must call `.lock()` or use
    /// `.with_lock()` to mutate or even read these fields.
    /// @{
    mutable mm::AddressSpace kernel_address_space;
    mutable TaskNamespace task_namespace;
    mutable di::Queue<di::Byte> input_data_queue;
    mutable WaitQueue input_wait_queue;
    mutable di::Queue<TaskFinalizationRequest> task_finalization_data_queue;
    mutable WaitQueue task_finalization_wait_queue;
    /// @}

    /// @name Per-processor fields
    /// Mutable global state which should really be per-processor, once SMP is supported.
    /// @{
    mutable Scheduler scheduler;
    /// @}
};

/// This function returns a mutable reference to the global state. This is only
/// valid during kernel boot, when no other tasks can execute.
GlobalState& global_state_in_boot();

/// This function returns a shared reference to the global state. This only provides
/// readonly access, and mutable fields must be protected using di::Synchronized.
inline GlobalState const& global_state() {
    return global_state_in_boot();
}
}
