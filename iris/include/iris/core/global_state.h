#pragma once

#include <di/prelude.h>
#include <iris/core/scheduler.h>
#include <iris/mm/address_space.h>

namespace iris {
struct GlobalState {
    constexpr GlobalState() {}
    constexpr ~GlobalState() {}

    // Read-only after after kernel initialization. Ideally would be marked const.
    mm::PhysicalAddress max_physical_address { 0 };
    mm::VirtualAddress virtual_to_physical_offset { 0 };
    mm::VirtualAddress heap_start { 0 };

    // Mutable global state. Should be protected with di::Syncrhonized in the future.
    mutable mm::AddressSpace kernel_address_space { 0 };
    mutable mm::VirtualAddress heap_end { 0 };

    // Mutable global state which should really be per-processor, once SMP is supported.
    mutable Scheduler scheduler;
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