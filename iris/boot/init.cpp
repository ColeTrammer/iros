#include <di/prelude.h>
#include <iris/arch/x86/amd64/idt.h>
#include <iris/arch/x86/amd64/segment_descriptor.h>
#include <iris/arch/x86/amd64/system_segment_descriptor.h>
#include <iris/arch/x86/amd64/tss.h>
#include <iris/boot/cxx_init.h>
#include <iris/core/log.h>
#include <iris/mm/address_space.h>
#include <iris/mm/map_physical_address.h>
#include <iris/mm/page_frame_allocator.h>
#include <iris/mm/sections.h>
#include <limine.h>

[[noreturn]] static void done() {
    for (;;) {
        asm volatile("mov $52, %eax\n"
                     "cli\n"
                     "hlt\n");
    }
    di::unreachable();
}

static void handler() {
    iris::debug_log(u8"Hello, World - from interrupt"_sv);
    done();
}

static void do_task() {
    iris::debug_log("Doing task after context switch"_sv);
    asm volatile("int $0x80");
    done();
}

struct [[gnu::packed]] IDTR {
    u16 size;
    u64 virtual_address;
};

static inline void load_idt(IDTR descriptor) {
    asm("lidtq %0" : : "m"(descriptor));
}

struct [[gnu::packed]] GDTR {
    u16 size;
    u64 virtual_address;
};

static inline void load_gdt(GDTR descriptor) {
    asm("lgdt %0" : : "m"(descriptor));
}

static inline void load_tr(u16 selector) {
    asm("ltr %0" : : "m"(selector));
}

static auto idt = di::Array<iris::x86::amd64::idt::Entry, 256> {};
static auto gdt = di::Array<iris::x86::amd64::sd::SegmentDescriptor, 11> {};
static auto tss = iris::x86::amd64::TSS {};

static inline void load_cr3(u64 cr3) {
    asm volatile("mov %0, %%rdx\n"
                 "mov %%rdx, %%cr3\n"
                 :
                 : "m"(cr3)
                 : "rdx");
}

extern "C" {
static volatile limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0,
    .response = nullptr,
};

static volatile limine_kernel_address_request kernel_address_request = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0,
    .response = nullptr,
};

static char __temp_stack[4 * 4096] alignas(4096);

void iris_main() {
    iris::arch::cxx_init();

    iris::debug_log(u8"Hello, World"_sv);

    {
        using namespace iris::x86::amd64::idt;

        auto handler_address = di::to_uintptr(&handler);
        auto pf_entry = Entry(Present(true), Type(Type::InterruptGate), SegmentSelector(5 * 8), TargetLow(handler_address & 0xFFFF),
                              TargetMid((handler_address >> 16) & 0xFFFF), TargetHigh(handler_address >> 32), DPL(3));
        for (auto& entry : idt) {
            entry = pf_entry;
        }

        auto idtr = IDTR { sizeof(idt) - 1, di::to_uintptr(idt.data()) };
        load_idt(idtr);
    }

    {
        using namespace iris::x86::amd64::ssd;

        // Setup TSS.
        tss.io_map_base = sizeof(tss);
        tss.rsp[0] = di::to_uintptr(__temp_stack);
        tss.rsp[1] = di::to_uintptr(__temp_stack);
        tss.rsp[2] = di::to_uintptr(__temp_stack);
        tss.ist[0] = di::to_uintptr(__temp_stack);

        // TSS Descriptor Setup.
        auto tss_descriptor = reinterpret_cast<SystemSegmentDescriptor*>(&gdt[9]);
        auto tss_address = di::to_uintptr(&tss);
        *tss_descriptor =
            SystemSegmentDescriptor(LimitLow(sizeof(tss)), BaseLow(tss_address & 0xFFFF), BaseMidLow((tss_address >> 16) & 0xFF),
                                    Type(Type::TSS), Present(true), BaseMidHigh((tss_address >> 24) & 0xFF), BaseHigh((tss_address >> 32)));
    }

    {
        using namespace iris::x86::amd64::sd;

        // The layout of the GDT matches the limine boot protocol, although this is not strictly necessary.
        // The 16 bit and 32 bit segments are included to ease future attempts to boot APs.
        // This is the null segment descriptor.
        gdt[0] = SegmentDescriptor();

        // 16 bit Code Descriptor.
        gdt[1] = SegmentDescriptor(LimitLow(0xFFFF), Readable(true), Code(true), DataOrCodeSegment(true), Present(true), LimitHigh(0xF),
                                   Granular(true));

        // 16 bit Data Descriptor.
        gdt[2] =
            SegmentDescriptor(LimitLow(0xFFFF), Writable(true), DataOrCodeSegment(true), Present(true), LimitHigh(0xF), Granular(true));

        // 32 bit Code Descriptor.
        gdt[3] = SegmentDescriptor(LimitLow(0xFFFF), Readable(true), Code(true), DataOrCodeSegment(true), Present(true), LimitHigh(0xF),
                                   Not16Bit(true), Granular(true));

        // 32 bit Data Descriptor.
        gdt[4] = SegmentDescriptor(LimitLow(0xFFFF), Writable(true), DataOrCodeSegment(true), Present(true), LimitHigh(0xF), Not16Bit(true),
                                   Granular(true));

        // 64 bit Code Descriptor.
        gdt[5] = SegmentDescriptor(LimitLow(0xFFFF), Readable(true), Code(true), DataOrCodeSegment(true), Present(true), LimitHigh(0xF),
                                   LongMode(true), Granular(true));

        // 64 bit Data Descriptor.
        gdt[6] = SegmentDescriptor(LimitLow(0xFFFF), Writable(true), DataOrCodeSegment(true), Present(true), LimitHigh(0xF), Not16Bit(true),
                                   Granular(true));

        // 64 bit User Code Descriptor.
        gdt[7] = SegmentDescriptor(LimitLow(0xFFFF), Readable(true), Code(true), DataOrCodeSegment(true), DPL(3), Present(true),
                                   LimitHigh(0xF), LongMode(true), Granular(true));

        // 64 bit User Data Descriptor.
        gdt[8] = SegmentDescriptor(LimitLow(0xFFFF), Writable(true), DataOrCodeSegment(true), DPL(3), Present(true), LimitHigh(0xF),
                                   Not16Bit(true), Granular(true));

        iris::debug_log("Gdt[5][0] = {:032b}\n"
                        "Gdt[5][1] = {:032b}"_sv,
                        reinterpret_cast<u32*>(gdt.data())[10], reinterpret_cast<u32*>(gdt.data())[11]);

        iris::debug_log("Gdt[7][0] = {:032b}\n"
                        "Gdt[7][1] = {:032b}"_sv,
                        reinterpret_cast<u32*>(gdt.data())[14], reinterpret_cast<u32*>(gdt.data())[15]);

        iris::debug_log("Gdt[6][0] = {:032b}\n"
                        "Gdt[6][1] = {:032b}"_sv,
                        reinterpret_cast<u32*>(gdt.data())[12], reinterpret_cast<u32*>(gdt.data())[13]);

        iris::debug_log("Gdt[8][0] = {:032b}\n"
                        "Gdt[8][1] = {:032b}"_sv,
                        reinterpret_cast<u32*>(gdt.data())[16], reinterpret_cast<u32*>(gdt.data())[17]);

        auto gdtr = GDTR { sizeof(gdt) - 1, di::to_uintptr(gdt.data()) };
        load_gdt(gdtr);

        // Load TSS.
        load_tr(9 * 8);

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

    iris::debug_log(u8"Hello, World - again"_sv);

    auto memory_map = di::Span { memmap_request.response->entries, memmap_request.response->entry_count };

    ASSERT(!memory_map.empty());
    auto max_physical_address = di::max(memory_map | di::transform([](auto* entry) {
                                            return entry->base + entry->length;
                                        }));
    (void) max_physical_address;

    for (auto* memory_map_entry : memory_map) {
        switch (memory_map_entry->type) {
            case LIMINE_MEMMAP_USABLE:
                iris::debug_log(u8"usable"_sv);
                break;
            case LIMINE_MEMMAP_RESERVED:
                iris::debug_log(u8"reserved"_sv);
                break;
            case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
                iris::debug_log(u8"ACPI reclaimable"_sv);
                break;
            case LIMINE_MEMMAP_ACPI_NVS:
                iris::debug_log(u8"ACPI NVS"_sv);
                break;
            case LIMINE_MEMMAP_BAD_MEMORY:
                iris::debug_log(u8"bad memory"_sv);
                break;
            case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
                iris::debug_log(u8"boot loader reclaimable"_sv);
                break;
            case LIMINE_MEMMAP_KERNEL_AND_MODULES:
                iris::debug_log(u8"kernel and modules"_sv);
                break;
            case LIMINE_MEMMAP_FRAMEBUFFER:
                iris::debug_log(u8"frame buffer"_sv);
                break;
            default:
                di::unreachable();
        }

        if (memory_map_entry->type != LIMINE_MEMMAP_USABLE) {
            iris::mm::reserve_page_frames(iris::mm::PhysicalAddress(memory_map_entry->base / 4096 * 4096),
                                          di::divide_round_up(memory_map_entry->length, 4096));
        }
    }

    iris::mm::reserve_page_frames(iris::mm::PhysicalAddress(0), 16 * 16 * 2);

    auto new_address_space = iris::mm::AddressSpace(iris::mm::allocate_page_frame()->raw_address());

    for (auto physical_address = iris::mm::PhysicalAddress(0); physical_address < iris::mm::PhysicalAddress(max_physical_address);
         physical_address += 0x1000) {
        (void) new_address_space.map_physical_page(iris::mm::VirtualAddress(0xFFFF800000000000 + physical_address.raw_address()),
                                                   physical_address);
    }

    for (auto virtual_address = iris::mm::text_segment_start; virtual_address < iris::mm::text_segment_end; virtual_address += 4096) {
        (void) new_address_space.map_physical_page(
            virtual_address, iris::mm::PhysicalAddress(kernel_address_request.response->physical_base +
                                                       (virtual_address.raw_address() - kernel_address_request.response->virtual_base)));
    }

    for (auto virtual_address = iris::mm::rodata_segment_start; virtual_address < iris::mm::rodata_segment_end; virtual_address += 4096) {
        (void) new_address_space.map_physical_page(
            virtual_address, iris::mm::PhysicalAddress(kernel_address_request.response->physical_base +
                                                       (virtual_address.raw_address() - kernel_address_request.response->virtual_base)));
    }

    for (auto virtual_address = iris::mm::data_segment_start; virtual_address < iris::mm::data_segment_end; virtual_address += 4096) {
        (void) new_address_space.map_physical_page(
            virtual_address, iris::mm::PhysicalAddress(kernel_address_request.response->physical_base +
                                                       (virtual_address.raw_address() - kernel_address_request.response->virtual_base)));
    }

    load_cr3(new_address_space.architecture_page_table_base());

    iris::debug_log(u8"Hello, World - again again"_sv);

    auto* x = new (std::nothrow) int { 42 };
    ASSERT(x != nullptr);
    delete x;

    iris::debug_log(u8"Hello, World - again again again"_sv);

    auto task_address = di::to_uintptr(&do_task);
    asm volatile("mov %0, %%rdx\n"
                 "push %1\n"
                 "push %2\n"
                 "push $0\n"
                 "push %3\n"
                 "push %%rdx\n"
                 "iretq\n"
                 :
                 : "r"(task_address), "i"(8 * 8 + 3), "r"(di::to_uintptr(__temp_stack)), "i"(7 * 8 + 3)
                 : "rdx", "memory");

    done();
}

void iris_entry() {
    asm volatile("mov %0, %%rsp\n"
                 "push $0\n"
                 "call iris_main\n"
                 :
                 : "r"(__temp_stack + sizeof(__temp_stack))
                 : "memory");
}
}