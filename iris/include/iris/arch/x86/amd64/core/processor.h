#pragma once

namespace iris {
class Processor;

/// @brief Get the current processor.
///
/// @warning This function is unsafe because it care is not taken, the current processor may change while the returned
/// reference is being used. Instead, use `current_processor()` unless you know preemption or interrupts are disabled.
inline Processor& current_processor_unsafe() {
    auto* result = static_cast<Processor*>(nullptr);
    asm volatile("mov %%gs:0, %0" : "=r"(result));
    return *result;
}
}
