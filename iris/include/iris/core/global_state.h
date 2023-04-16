#pragma once

#include <di/prelude.h>
#include <iris/core/config.h>
#include <iris/core/error.h>
#include <iris/core/object_pool.h>
#include <iris/core/processor.h>
#include <iris/core/scheduler.h>
#include <iris/core/task.h>
#include <iris/core/task_namespace.h>
#include <iris/core/unit_test.h>
#include <iris/hw/acpi/acpi.h>
#include <iris/hw/irq.h>
#include <iris/hw/timer.h>
#include <iris/mm/address_space.h>

#include IRIS_ARCH_INCLUDE(hw/processor_info.h)
#include IRIS_ARCH_INCLUDE(core/global_state.h)

namespace iris {
struct GlobalState {
    GlobalState() = default;
    ~GlobalState() = default;

    /// @name Readonly fields
    /// Read-only after after kernel initialization. Ideally would be marked const.
    /// @{
    mm::PhysicalAddress max_physical_address { 0 };
    mm::VirtualAddress virtual_to_physical_offset { 0 };
    mm::VirtualAddress heap_start { 0 };
    di::Span<byte const> initrd;
    ProcessorInfo processor_info;
    test::TestManager unit_test_manager;
    arch::FpuState initial_fpu_state;
    di::Optional<acpi::AcpiInformation> acpi_info;
    Processor boot_processor;
    di::LinkedList<Processor> alernate_processors;
    di::TreeMap<u32, Processor*> processor_map;
    arch::ReadonlyGlobalState arch_readonly_state;
    mutable di::LinkedList<di::Synchronized<IrqController>> irq_controllers;
    mutable di::LinkedList<di::Synchronized<Timer>> timers;
    mutable di::Synchronized<Timer>* scheduler_timer { nullptr };
    mutable di::Synchronized<Timer>* calibration_timer { nullptr };
    bool current_processor_available { false };
    /// @}

    /// @name Mutable fields
    /// Mutable global state. These fields have internal synchronization, and so are
    /// safe to access concurrently. Typically, calling code must call `.lock()` or use
    /// `.with_lock()` to mutate or even read these fields.
    /// @{
    mutable mm::AddressSpace kernel_address_space;
    mutable TaskNamespace task_namespace;
    mutable di::Queue<byte, di::StaticRing<byte, di::meta::SizeConstant<128>>> input_data_queue;
    mutable WaitQueue input_wait_queue;
    mutable di::Queue<TaskFinalizationRequest, di::StaticRing<TaskFinalizationRequest, di::meta::SizeConstant<128>>>
        task_finalization_data_queue;
    mutable WaitQueue task_finalization_wait_queue;
    mutable di::Synchronized<di::Array<di::StaticVector<IrqHandler, di::meta::SizeConstant<8>>, 256>> irq_handlers;
    mutable arch::MutableGlobalState arch_mutable_state;
    mutable di::Atomic<bool> all_aps_booted { false };
    mutable Spinlock debug_output_lock;
    mutable di::Synchronized<ObjectPool<IpiMessage>> ipi_message_pool;
    mutable di::Atomic<u32> next_processor_to_schedule_on { 0 };
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
