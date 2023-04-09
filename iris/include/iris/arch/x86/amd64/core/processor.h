#pragma once

#include <iris/arch/x86/amd64/hw/local_apic.h>

namespace iris {
class Processor;

namespace arch {
    class ArchProcessor {
    public:
        /// @brief Get the local APIC.
        ///
        /// @warning This function is only safe to call if `global_state().arch_readonly_state.use_apic` is `true`, and
        /// the local APIC has been initialized.
        x86::amd64::LocalApic& local_apic() { return *m_local_apic; }
        void set_local_apic(x86::amd64::LocalApic local_apic) { m_local_apic = local_apic; }

    private:
        di::Optional<x86::amd64::LocalApic> m_local_apic;
    };
}

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
