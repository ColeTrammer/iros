#pragma once

#include <di/prelude.h>

#include <iris/core/preemption.h>
#include <iris/core/scheduler.h>

#include IRIS_ARCH_INCLUDE(core/processor.h)

namespace iris {
class Processor : public di::SelfPointer<Processor> {
public:
    Scheduler& scheduler() { return m_scheduler; }

    arch::ArchProcessor& arch_processor() { return m_arch_processor; }

private:
    Scheduler m_scheduler;
    arch::ArchProcessor m_arch_processor;
};

/// @brief Setups access to the current processor.
///
/// This function is called whenever a context switch from lower privilege levels to higher privilege levels is
/// performed. On x86_64, this loads the GS segment register with the address of the current processor using `swapgs`.
void setup_current_processor_access();

/// @brief Sets the current processor address.
///
/// @warning This function can only be called once, during each processor's initialization.
void set_current_processor(Processor& processor);

inline auto current_processor() {
    auto guard = PreemptionDisabler {};
    // SAFETY: This is safe since preemption is disabled.
    auto& processor = current_processor_unsafe();
    return di::GuardedReference<Processor, PreemptionDisabler> { processor, di::move(guard) };
}

inline auto current_scheduler() {
    auto guard = PreemptionDisabler {};
    // SAFETY: This is safe since preemption is disabled.
    auto& processor = current_processor_unsafe();
    return di::GuardedReference<Scheduler, PreemptionDisabler> { processor.scheduler(), di::move(guard) };
}
}
