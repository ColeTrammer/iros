#include <iris/arch/x86/amd64/gdt.h>
#include <iris/arch/x86/amd64/msr.h>
#include <iris/arch/x86/amd64/system_instructions.h>
#include <iris/core/global_state.h>

namespace iris {
void setup_current_processor_access() {
    x86::amd64::swapgs();
}

void set_current_processor(Processor& processor) {
    if (global_state().processor_info.has_fs_gs_base()) {
        x86::amd64::write_gs_base(reinterpret_cast<uptr>(&processor));
    } else {
        x86::amd64::write_msr(x86::amd64::ModelSpecificRegister::GsBase, reinterpret_cast<uptr>(&processor));
    }
}
}

namespace iris::arch {
void load_userspace_thread_pointer(uptr userspace_thread_pointer, arch::TaskState& task_state) {
    if (!task_state.in_kernel()) {
        x86::amd64::swapgs();

        if (global_state().processor_info.has_fs_gs_base()) {
            x86::amd64::write_fs_base(userspace_thread_pointer);
            x86::amd64::write_gs_base(userspace_thread_pointer);
        } else {
            x86::amd64::write_msr(x86::amd64::ModelSpecificRegister::FsBase, userspace_thread_pointer);
            x86::amd64::write_msr(x86::amd64::ModelSpecificRegister::GsBase, userspace_thread_pointer);
        }
    }
}
}

namespace iris::x86::amd64 {
void init_gdt() {
    using namespace iris::x86::amd64::sd;

    // SAFETY: This is safe since this function is only called by starting up a processor.
    auto& gdt = current_processor_unsafe().arch_processor().gdt();

    // The layout of the GDT matches the limine boot protocol, although this is not strictly necessary.
    // The 16 bit and 32 bit segments are included to ease future attempts to boot APs.
    // This is the null segment descriptor.
    gdt[0] = SegmentDescriptor();

    // 16 bit Code Descriptor.
    gdt[1] = SegmentDescriptor(LimitLow(0xFFFF), Readable(true), Code(true), DataOrCodeSegment(true), Present(true),
                               LimitHigh(0xF), Granular(true));

    // 16 bit Data Descriptor.
    gdt[2] = SegmentDescriptor(LimitLow(0xFFFF), Writable(true), DataOrCodeSegment(true), Present(true), LimitHigh(0xF),
                               Granular(true));

    // 32 bit Code Descriptor.
    gdt[3] = SegmentDescriptor(LimitLow(0xFFFF), Readable(true), Code(true), DataOrCodeSegment(true), Present(true),
                               LimitHigh(0xF), Not16Bit(true), Granular(true));

    // 32 bit Data Descriptor.
    gdt[4] = SegmentDescriptor(LimitLow(0xFFFF), Writable(true), DataOrCodeSegment(true), Present(true), LimitHigh(0xF),
                               Not16Bit(true), Granular(true));

    // 64 bit Code Descriptor.
    gdt[5] = SegmentDescriptor(LimitLow(0xFFFF), Readable(true), Code(true), DataOrCodeSegment(true), Present(true),
                               LimitHigh(0xF), LongMode(true), Granular(true));

    // 64 bit Data Descriptor.
    gdt[6] = SegmentDescriptor(LimitLow(0xFFFF), Writable(true), DataOrCodeSegment(true), Present(true), LimitHigh(0xF),
                               Not16Bit(true), Granular(true));

    // 64 bit User Code Descriptor.
    gdt[7] = SegmentDescriptor(LimitLow(0xFFFF), Readable(true), Code(true), DataOrCodeSegment(true), DPL(3),
                               Present(true), LimitHigh(0xF), LongMode(true), Granular(true));

    // 64 bit User Data Descriptor.
    gdt[8] = SegmentDescriptor(LimitLow(0xFFFF), Writable(true), DataOrCodeSegment(true), DPL(3), Present(true),
                               LimitHigh(0xF), Not16Bit(true), Granular(true));

    auto gdtr = iris::x86::amd64::GDTR { sizeof(gdt) - 1, di::to_uintptr(gdt.data()) };
    iris::x86::amd64::load_gdt(gdtr);

    // Load TSS.
    iris::x86::amd64::load_tr(9 * 8);

    // Load the data segments with NULL segment selector.
    asm volatile("mov %0, %%dx\n"
                 "mov %%dx, %%ds\n"
                 "mov %%dx, %%es\n"
                 "mov %%dx, %%fs\n"
                 "mov %%dx, %%ss\n"
                 "mov %%dx, %%gs\n"
                 :
                 : "i"(0)
                 : "memory", "edx");
}
}
