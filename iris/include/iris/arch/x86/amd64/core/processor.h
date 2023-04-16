#pragma once

#include <iris/arch/x86/amd64/hw/local_apic.h>
#include <iris/arch/x86/amd64/segment_descriptor.h>
#include <iris/arch/x86/amd64/tss.h>
#include <iris/hw/irq.h>

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

        auto& gdt() { return m_gdt; }
        auto& tss() { return m_tss; }

        void set_fallback_kernel_stack(uptr stack) { m_fallback_kernel_stack = stack; }
        uptr fallback_kernel_stack() const { return m_fallback_kernel_stack; }

        /// @brief Setup the processor to allow floating-point / SIMD operations.
        ///
        /// @note This must be called once at boot for each logical processor.
        void setup_fpu_support_for_processor(bool print_info = true);

        /// @brief Enable CPU features for the current processor.
        ///
        /// @note This must be called once at boot for each logical processor.
        void enable_cpu_features(bool print_info = true);

        void set_local_apic_callback(di::Function<void(IrqContext&)> callback) {
            m_local_apic_callback = di::move(callback);
        }
        void local_apic_callback(IrqContext& context) { m_local_apic_callback(context); }

    private:
        di::Optional<x86::amd64::LocalApic> m_local_apic;
        di::Array<iris::x86::amd64::sd::SegmentDescriptor, 11> m_gdt {};
        iris::x86::amd64::TSS m_tss {};
        uptr m_fallback_kernel_stack {};
        di::Function<void(IrqContext&)> m_local_apic_callback;
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
