#include <di/prelude.h>
#include <iris/arch/x86/amd64/idt.h>
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
    iris::debug_log("Hello, World - from interrupt"_sv);
    done();
}

struct [[gnu::packed]] IDTR {
    u16 size;
    u64 virtual_address;
};

static inline void load_idt(IDTR descriptor) {
    asm("lidtq %0" : : "m"(descriptor));
}

static auto idt = di::Array<iris::x86::amd64::idt::Entry, 256> {};

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

static char __temp_stack[4 * 4096];

void iris_main() {
    asm volatile("mov %0, %%rsp" : : "r"(__temp_stack + sizeof(__temp_stack)) : "memory");

    iris::arch::cxx_init();

    iris::debug_log("Hello, World"_sv);

    using namespace iris::x86::amd64::idt;

    auto handler_address = reinterpret_cast<u64>(&handler);
    auto pf_entry = Entry(Present(true), Type(Type::InterruptGate), SegmentSelector(0x28), TargetLow(handler_address & 0xFFFF),
                          TargetMid((handler_address >> 16) & 0xFFFF), TargetHigh(handler_address >> 32));
    for (auto& entry : idt) {
        entry = pf_entry;
    }

    auto idtr = IDTR { sizeof(idt) - 1, reinterpret_cast<u64>(idt.data()) };
    load_idt(idtr);

    iris::debug_log("Hello, World - again"_sv);

    auto memory_map = di::Span { memmap_request.response->entries, memmap_request.response->entry_count };

    ASSERT(!memory_map.empty());
    auto max_physical_address = di::max(memory_map | di::transform([](auto* entry) {
                                            return entry->base + entry->length;
                                        }));
    (void) max_physical_address;

    for (auto* memory_map_entry : memory_map) {
        switch (memory_map_entry->type) {
            case LIMINE_MEMMAP_USABLE:
                iris::debug_log("usable"_sv);
                break;
            case LIMINE_MEMMAP_RESERVED:
                iris::debug_log("reserved"_sv);
                break;
            case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
                iris::debug_log("ACPI reclaimable"_sv);
                break;
            case LIMINE_MEMMAP_ACPI_NVS:
                iris::debug_log("ACPI NVS"_sv);
                break;
            case LIMINE_MEMMAP_BAD_MEMORY:
                iris::debug_log("bad memory"_sv);
                break;
            case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
                iris::debug_log("boot loader reclaimable"_sv);
                break;
            case LIMINE_MEMMAP_KERNEL_AND_MODULES:
                iris::debug_log("kernel and modules"_sv);
                break;
            case LIMINE_MEMMAP_FRAMEBUFFER:
                iris::debug_log("frame buffer"_sv);
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

    iris::debug_log("Hello, World - again again"_sv);

    auto* x = new (std::nothrow) int { 42 };
    ASSERT(x != nullptr);
    delete x;

    iris::debug_log("Hello, World - again again again"_sv);

    done();
}
}