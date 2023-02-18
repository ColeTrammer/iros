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

static auto userspace_test_program_data_storage = di::Array<di::Byte, 5 * 0x4000> {};

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

        if (memory_map_entry->type != LIMINE_MEMMAP_USABLE) {
            iris::mm::reserve_page_frames(iris::mm::PhysicalAddress(di::align_down(memory_map_entry->base, 4096)),
                                          di::divide_round_up(memory_map_entry->length, 4096));
        }
    }

    iris::mm::reserve_page_frames(iris::mm::PhysicalAddress(0), 16 * 16 * 2);

    DI_ASSERT_GT(module_request.response->module_count, 0u);
    auto userspace_test_program = *module_request.response->modules[0];
    DI_ASSERT_LT_EQ(userspace_test_program.size, userspace_test_program_data_storage.size());
    di::copy(
        di::Span { reinterpret_cast<di::Byte const*>(userspace_test_program.address), userspace_test_program.size },
        userspace_test_program_data_storage.data());
    auto test_program_data = di::Span { userspace_test_program_data_storage.data(), userspace_test_program.size };

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

    auto* elf_header = test_program_data.typed_pointer_unchecked<elf64::ElfHeader>(0);
    iris::println("entry={:x}"_sv, elf_header->entry);
    ASSERT_EQ(sizeof(elf64::ProgramHeader), elf_header->program_entry_size);

    auto program_headers = test_program_data.typed_span_unchecked<elf64::ProgramHeader>(
        elf_header->program_table_off, elf_header->program_entry_count);
    for (auto& program_header : program_headers) {
        iris::println("type={}"_sv, program_header.type);
        iris::println("addr={:x}"_sv, program_header.virtual_addr);
        iris::println("file={:x}"_sv, program_header.file_size);
        iris::println("memory={:x}"_sv, program_header.memory_size);

        // PT_LOAD
        if (program_header.type != 1) {
            continue;
        }

        auto aligned_size = di::align_up(program_header.memory_size, 4096);
        (void) kernel_address_space.allocate_region_at(iris::mm::VirtualAddress(program_header.virtual_addr),
                                                       aligned_size);

        iris::println("copy..."_sv);
        auto data = di::Span { reinterpret_cast<di::Byte*>(program_header.virtual_addr), aligned_size };
        di::copy(*test_program_data.subspan(program_header.offset, program_header.memory_size), data.data());
        iris::println("done"_sv);
    }

    auto task_stack4 = *kernel_address_space.allocate_region(0x2000);
    auto task4 = iris::Task(elf_header->entry, task_stack4.raw_address(), true);

    (void) task4;
    scheduler.schedule_task(task4);

    iris::println("preparing to context switch"_sv);

    iris::println("stack1={:x}"_sv, task_stack1.raw_address());
    iris::println("stack2={:x}"_sv, task_stack2.raw_address());
    iris::println("stack3={:x}"_sv, task_stack3.raw_address());

    scheduler.start();

    done();
}
}