#include <di/prelude.h>
#include <iris/arch/x86/amd64/idt.h>
#include <iris/arch/x86/amd64/segment_descriptor.h>
#include <iris/arch/x86/amd64/system_instructions.h>
#include <iris/arch/x86/amd64/system_segment_descriptor.h>
#include <iris/arch/x86/amd64/tss.h>
#include <iris/boot/cxx_init.h>
#include <iris/boot/init.h>
#include <iris/core/global_state.h>
#include <iris/core/print.h>
#include <iris/core/scheduler.h>
#include <iris/core/task.h>
#include <iris/mm/address_space.h>
#include <iris/mm/map_physical_address.h>
#include <iris/mm/page_frame_allocator.h>
#include <iris/mm/sections.h>
#include <limine.h>

namespace elf64 {
using Addr = u64;
using Off = i64;
using Half = u16;
using Word = u32;
using Sword = i32;
using Xword = u64;
using Sxword = i64;
using Byte = di::Byte;

struct ElfHeader {
    Byte ident[16];
    Half type;
    Half machine;
    Word version;
    Addr entry;
    Off program_table_off;
    Off section_table_off;
    Word flags;
    Half elf_header_size;
    Half program_entry_size;
    Half program_entry_count;
    Half section_entry_size;
    Half section_entry_count;
    Half string_table_section_index;
};

struct ProgramHeader {
    Word type;
    Word flags;
    Off offset;
    Addr virtual_addr;
    Addr physical_addr;
    Xword file_size;
    Xword memory_size;
    Xword align;
};
}

[[noreturn]] static void done() {
    for (;;) {
        asm volatile("mov $52, %eax\n"
                     "cli\n"
                     "hlt\n");
    }
    di::unreachable();
}

static int counter = 0;

static void do_task() {
    for (int i = 0; i < 3; i++) {
        iris::println("counter: {}"_sv, ++counter);
        asm volatile("sti\nhlt");
    }
    done();
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

static volatile limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0,
    .response = nullptr,
};
}

namespace iris {
void iris_main() {
    iris::println(u8"Hello, World - again"_sv);

    auto& global_state = global_state_in_boot();
    global_state.heap_start = iris::mm::VirtualAddress(di::align_up(iris::mm::kernel_end.raw_address(), 4096));
    global_state.heap_end = global_state.heap_start;

    auto memory_map = di::Span { memmap_request.response->entries, memmap_request.response->entry_count };

    ASSERT(!memory_map.empty());
    global_state.max_physical_address = di::max(memory_map | di::transform([](auto* entry) {
                                                    return mm::PhysicalAddress(entry->base) + entry->length;
                                                }));

    for (auto* memory_map_entry : memory_map) {
        switch (memory_map_entry->type) {
            case LIMINE_MEMMAP_USABLE:
                iris::println(u8"usable"_sv);
                break;
            case LIMINE_MEMMAP_RESERVED:
                iris::println(u8"reserved"_sv);
                break;
            case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
                iris::println(u8"ACPI reclaimable"_sv);
                break;
            case LIMINE_MEMMAP_ACPI_NVS:
                iris::println(u8"ACPI NVS"_sv);
                break;
            case LIMINE_MEMMAP_BAD_MEMORY:
                iris::println(u8"bad memory"_sv);
                break;
            case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
                iris::println(u8"boot loader reclaimable"_sv);
                break;
            case LIMINE_MEMMAP_KERNEL_AND_MODULES:
                iris::println(u8"kernel and modules"_sv);
                break;
            case LIMINE_MEMMAP_FRAMEBUFFER:
                iris::println(u8"frame buffer"_sv);
                break;
            default:
                di::unreachable();
        }

        iris::println("base={:#018x} size={:#x}"_sv, memory_map_entry->base, memory_map_entry->length);

        if (memory_map_entry->type != LIMINE_MEMMAP_USABLE) {
            iris::mm::reserve_page_frames(iris::mm::PhysicalAddress(di::align_down(memory_map_entry->base, 4096)),
                                          di::divide_round_up(memory_map_entry->length, 4096));
        }
    }

    iris::mm::reserve_page_frames(iris::mm::PhysicalAddress(0), 16 * 16 * 2);

    DI_ASSERT_GT(module_request.response->module_count, 0u);
    auto initrd_module = *module_request.response->modules[0];

    iris::println("Kernel virtual base: {:#018x}"_sv, kernel_address_request.response->virtual_base);
    iris::println("Kernel physical base: {:#018x}"_sv, kernel_address_request.response->physical_base);
    iris::println("Max physical memory: {:#018x}"_sv, global_state.max_physical_address.raw_address());
    iris::println("Module base address: {}"_sv, initrd_module.address);

    // NOTE: the limine boot loader places the module in the HHDM, and marks the region and kernel+modules,
    //       so we can safely access its provided virtual address after unloading the boot loader. This relies
    //       on limine placing its HHDM at the same location we do, which is unsafe but works for now.
    auto initrd = di::Span { reinterpret_cast<di::Byte const*>(initrd_module.address), initrd_module.size };

    ASSERT(mm::init_and_load_initial_kernel_address_space(
        mm::PhysicalAddress(kernel_address_request.response->physical_base),
        mm::VirtualAddress(kernel_address_request.response->virtual_base), global_state.max_physical_address));

    iris::println(u8"Hello, World - again again"_sv);

    auto* x = new (std::nothrow) int { 42 };
    ASSERT(x != nullptr);
    delete x;

    iris::println(u8"Hello, World - again again again"_sv);

    auto task_address = di::to_uintptr(&do_task);

    auto& kernel_address_space = global_state.kernel_address_space;
    auto task_stack1 = *kernel_address_space.allocate_region(0x2000);
    auto task1 = iris::Task(task_address, task_stack1.raw_address() + 0x2000, false);

    auto& scheduler = global_state.scheduler;
    scheduler.schedule_task(task1);

    auto task_stack2 = *kernel_address_space.allocate_region(0x2000);
    auto task2 = iris::Task(task_address, task_stack2.raw_address() + 0x2000, false);

    scheduler.schedule_task(task2);

    auto task_stack3 = *kernel_address_space.allocate_region(0x2000);
    auto task3 = iris::Task(task_address, task_stack3.raw_address() + 0x2000, false);

    scheduler.schedule_task(task3);

    iris::println("preparing to context switch"_sv);

    iris::println("stack1={:x}"_sv, task_stack1.raw_address());
    iris::println("stack2={:x}"_sv, task_stack2.raw_address());
    iris::println("stack3={:x}"_sv, task_stack3.raw_address());

    scheduler.start();

    done();
}
}