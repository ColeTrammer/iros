#include <iris/arch/x86/amd64/gdt.h>
#include <iris/arch/x86/amd64/msr.h>
#include <iris/arch/x86/amd64/system_instructions.h>
#include <iris/arch/x86/amd64/system_segment_descriptor.h>
#include <iris/core/global_state.h>

namespace iris::arch {
void load_kernel_stack(mm::VirtualAddress base) {
    // SAFETY: This function must be called with interrupts disabled.
    current_processor_unsafe().arch_processor().tss().rsp[0] = base.raw_value();
}
}

namespace iris::x86::amd64 {
void init_tss() {
    using namespace iris::x86::amd64::ssd;

    // SAFETY: This is safe since this function is only called by starting up a processor.
    auto fallback_kernel_stack = current_processor_unsafe().arch_processor().fallback_kernel_stack();
    auto& tss = current_processor_unsafe().arch_processor().tss();
    auto& gdt = current_processor_unsafe().arch_processor().gdt();

    // Setup TSS.
    tss.io_map_base = sizeof(tss);
    tss.rsp[0] = fallback_kernel_stack;
    tss.rsp[1] = fallback_kernel_stack;
    tss.rsp[2] = fallback_kernel_stack;
    tss.ist[0] = fallback_kernel_stack;

    // TSS Descriptor Setup.
    auto* tss_descriptor = reinterpret_cast<SystemSegmentDescriptor*>(&gdt[9]);
    auto tss_address = di::to_uintptr(&tss);
    *tss_descriptor = SystemSegmentDescriptor(LimitLow(sizeof(tss)), BaseLow(tss_address & 0xFFFF),
                                              BaseMidLow((tss_address >> 16) & 0xFF), Type(Type::TSS), Present(true),
                                              BaseMidHigh((tss_address >> 24) & 0xFF), BaseHigh((tss_address >> 32)));
}
}
