#include <di/prelude.h>
#include <iris/arch/x86/amd64/hw/pic.h>
#include <iris/arch/x86/amd64/hw/serial.h>
#include <iris/arch/x86/amd64/idt.h>
#include <iris/arch/x86/amd64/io_instructions.h>
#include <iris/arch/x86/amd64/msr.h>
#include <iris/arch/x86/amd64/segment_descriptor.h>
#include <iris/arch/x86/amd64/system_instructions.h>
#include <iris/arch/x86/amd64/system_segment_descriptor.h>
#include <iris/arch/x86/amd64/tss.h>
#include <iris/boot/cxx_init.h>
#include <iris/boot/init.h>
#include <iris/core/global_state.h>
#include <iris/core/interrupt_disabler.h>
#include <iris/core/print.h>
#include <iris/core/scheduler.h>
#include <iris/core/task.h>
#include <iris/core/userspace_access.h>
#include <iris/core/wait_queue.h>
#include <iris/fs/debug_file.h>
#include <iris/fs/file.h>
#include <iris/fs/initrd.h>
#include <iris/hw/power.h>
#include <iris/mm/address_space.h>
#include <iris/mm/map_physical_address.h>
#include <iris/mm/page_frame_allocator.h>
#include <iris/mm/sections.h>
#include <iris/uapi/syscall.h>

namespace iris {
Expected<usize> tag_invoke(di::Tag<read_file>, DebugFile&, di::Span<di::Byte> buffer) {
    if (buffer.empty()) {
        return 0;
    }

    auto byte = di::Byte(0);
    TRY(global_state().input_wait_queue.wait([&] {
        auto has_data = global_state().input_data_queue.pop();
        if (has_data) {
            byte = *has_data;
            return true;
        }
        return false;
    }));

    buffer[0] = byte;
    return 1;
}
}

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
alignas(4096) static char temp_stack[4 * 4096];

static auto gdt = di::Array<iris::x86::amd64::sd::SegmentDescriptor, 11> {};
static auto tss = iris::x86::amd64::TSS {};

void load_kernel_stack(mm::VirtualAddress base) {
    tss.rsp[0] = base.raw_value();
}

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

extern "C" void bsp_cpu_init() {
    iris::arch::cxx_init();

    iris::x86::amd64::init_serial_early_boot();

    auto& global_state = global_state_in_boot();

    iris::println("Beginning x86_64 kernel boot..."_sv);

    global_state.processor_info = detect_processor_info();
    global_state.processor_info.print_to_console();

    if (!!(global_state.processor_info.features & ProcessorFeatures::Smep)) {
        iris::println("Enabling SMEP..."_sv);
        x86::amd64::load_cr4(x86::amd64::read_cr4() | (1 << 20));
    }

    if (!!(global_state.processor_info.features & ProcessorFeatures::Smap)) {
        iris::println("Enabling SMAP..."_sv);
        x86::amd64::load_cr4(x86::amd64::read_cr4() | (1 << 21));
    }

    if (global_state.processor_info.has_fs_gs_base()) {
        iris::println("Enabling FS/GS Base..."_sv);
        x86::amd64::load_cr4(x86::amd64::read_cr4() | (1 << 16));
    }

    x86::amd64::idt::init_idt();

    {
        using namespace iris::x86::amd64::ssd;

        // Setup TSS.
        tss.io_map_base = sizeof(tss);
        tss.rsp[0] = di::to_uintptr(temp_stack);
        tss.rsp[1] = di::to_uintptr(temp_stack);
        tss.rsp[2] = di::to_uintptr(temp_stack);
        tss.ist[0] = di::to_uintptr(temp_stack);

        // TSS Descriptor Setup.
        auto tss_descriptor = reinterpret_cast<SystemSegmentDescriptor*>(&gdt[9]);
        auto tss_address = di::to_uintptr(&tss);
        *tss_descriptor = SystemSegmentDescriptor(
            LimitLow(sizeof(tss)), BaseLow(tss_address & 0xFFFF), BaseMidLow((tss_address >> 16) & 0xFF),
            Type(Type::TSS), Present(true), BaseMidHigh((tss_address >> 24) & 0xFF), BaseHigh((tss_address >> 32)));
    }

    {
        using namespace iris::x86::amd64::sd;

        // The layout of the GDT matches the limine boot protocol, although this is not strictly necessary.
        // The 16 bit and 32 bit segments are included to ease future attempts to boot APs.
        // This is the null segment descriptor.
        gdt[0] = SegmentDescriptor();

        // 16 bit Code Descriptor.
        gdt[1] = SegmentDescriptor(LimitLow(0xFFFF), Readable(true), Code(true), DataOrCodeSegment(true), Present(true),
                                   LimitHigh(0xF), Granular(true));

        // 16 bit Data Descriptor.
        gdt[2] = SegmentDescriptor(LimitLow(0xFFFF), Writable(true), DataOrCodeSegment(true), Present(true),
                                   LimitHigh(0xF), Granular(true));

        // 32 bit Code Descriptor.
        gdt[3] = SegmentDescriptor(LimitLow(0xFFFF), Readable(true), Code(true), DataOrCodeSegment(true), Present(true),
                                   LimitHigh(0xF), Not16Bit(true), Granular(true));

        // 32 bit Data Descriptor.
        gdt[4] = SegmentDescriptor(LimitLow(0xFFFF), Writable(true), DataOrCodeSegment(true), Present(true),
                                   LimitHigh(0xF), Not16Bit(true), Granular(true));

        // 64 bit Code Descriptor.
        gdt[5] = SegmentDescriptor(LimitLow(0xFFFF), Readable(true), Code(true), DataOrCodeSegment(true), Present(true),
                                   LimitHigh(0xF), LongMode(true), Granular(true));

        // 64 bit Data Descriptor.
        gdt[6] = SegmentDescriptor(LimitLow(0xFFFF), Writable(true), DataOrCodeSegment(true), Present(true),
                                   LimitHigh(0xF), Not16Bit(true), Granular(true));

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

    set_current_processor(global_state.boot_processor);

    iris::x86::amd64::init_pic();

    iris_main();
}

extern "C" [[gnu::naked]] void iris_entry() {
    asm volatile("mov %0, %%rsp\n"
                 "push $0\n"
                 "call bsp_cpu_init\n"
                 :
                 : "r"(temp_stack + sizeof(temp_stack))
                 : "memory");
}
}
