#pragma once

#include <di/prelude.h>

#include <iris/core/scheduler.h>

#include IRIS_ARCH_INCLUDE(core/processor.h)

namespace iris {
class Processor : public di::SelfPointer<Processor> {
public:
    Scheduler& scheduler() { return m_scheduler; }

private:
    Scheduler m_scheduler;
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

inline Scheduler& current_scheduler() {
    return current_processor().scheduler();
}
}
