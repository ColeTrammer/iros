#include <iris/arch/x86/amd64/system_instructions.h>
#include <iris/core/global_state.h>
#include <iris/core/print.h>
#include <iris/core/processor.h>

namespace iris::arch {
void ArchProcessor::setup_fpu_support_for_processor(bool print_info) {
    // Clear cr0.EM and set cr0.MP.
    auto cr0 = x86::amd64::read_cr0();
    cr0 &= ~(1 << 2);
    cr0 |= (1 << 1);
    x86::amd64::load_cr0(cr0);

    // Enable legacy SSE, which is guaranteed to be supported on x86_64.
    // This sets cr4.OSFXSR and cr4.OSXMMEXCPT.
    auto cr4 = x86::amd64::read_cr4();
    cr4 |= (1 << 9) | (1 << 10);
    x86::amd64::load_cr4(cr4);

    // Create clean copy of FPU state.
    x86::amd64::fninit();

    // See if the processor supports CPU extensions.
    if (global_state().processor_info.has_xsave()) {
        if (print_info) {
            println("Enabling support for extended SSE instructions..."_sv);
        }

        // Set cr4.OSXSAVE.
        cr4 |= (1 << 18);
        x86::amd64::load_cr4(cr4);

        // Set xcr0 register as the processor desired.
        x86::amd64::xsetbv(0, global_state().processor_info.fpu_valid_xcr0);
    }
}

void ArchProcessor::enable_cpu_features(bool print_info) {
    auto& global_state = iris::global_state();
    if (!!(global_state.processor_info.features & ProcessorFeatures::Smep)) {
        if (print_info) {
            iris::println("Enabling SMEP..."_sv);
        }
        x86::amd64::load_cr4(x86::amd64::read_cr4() | (1 << 20));
    }

    if (!!(global_state.processor_info.features & ProcessorFeatures::Smap)) {
        if (print_info) {
            iris::println("Enabling SMAP..."_sv);
        }
        x86::amd64::load_cr4(x86::amd64::read_cr4() | (1 << 21));
    }

    if (global_state.processor_info.has_fs_gs_base()) {
        if (print_info) {
            iris::println("Enabling FS/GS Base..."_sv);
        }
        x86::amd64::load_cr4(x86::amd64::read_cr4() | (1 << 16));
    }
}
}
