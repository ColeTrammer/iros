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
#include <iris/fs/initrd.h>
#include <iris/mm/address_space.h>
#include <iris/mm/map_physical_address.h>
#include <iris/mm/page_frame_allocator.h>
#include <iris/mm/sections.h>
#include <limine.h>

static int counter = 0;

static void do_task() {
    for (int i = 0; i < 3; i++) {
        asm volatile("cli");
        iris::println("counter: {}"_sv, ++counter);
        asm volatile("sti\nhlt");
    }
    iris::global_state().scheduler.exit_current_task();
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
    iris::println("Starting architecture independent initialization..."_sv);

    auto& global_state = global_state_in_boot();
    global_state.heap_start = iris::mm::VirtualAddress(di::align_up(iris::mm::kernel_end.raw_value(), 4096));
    global_state.heap_end = global_state.heap_start;

    auto memory_map = di::Span { memmap_request.response->entries, memmap_request.response->entry_count };

    ASSERT(!memory_map.empty());
    global_state.max_physical_address = di::max(memory_map | di::transform([](auto* entry) {
                                                    return mm::PhysicalAddress(entry->base) + entry->length;
                                                }));

    for (auto* memory_map_entry : memory_map) {
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
    iris::println("Max physical memory: {}"_sv, global_state.max_physical_address);
    iris::println("Module base address: {}"_sv, initrd_module.address);

    // NOTE: the limine boot loader places the module in the HHDM, and marks the region and kernel+modules,
    //       so we can safely access its provided virtual address after unloading the boot loader. This relies
    //       on limine placing its HHDM at the same location we do, which is unsafe but works for now.
    global_state.initrd = di::Span { static_cast<di::Byte const*>(initrd_module.address), initrd_module.size };

    ASSERT(mm::init_and_load_initial_kernel_address_space(
        mm::PhysicalAddress(kernel_address_request.response->physical_base),
        mm::VirtualAddress(kernel_address_request.response->virtual_base), global_state.max_physical_address));

    auto& scheduler = global_state.scheduler;
    {
        auto task1 = *iris::create_kernel_task(global_state.task_namespace, do_task);
        auto task2 = *iris::create_kernel_task(global_state.task_namespace, do_task);
        auto task3 = *iris::create_kernel_task(global_state.task_namespace, do_task);

        scheduler.schedule_task(*task1);
        scheduler.schedule_task(*task2);
        scheduler.schedule_task(*task3);

        auto task4 = *iris::create_user_task(global_state.task_namespace);

        auto init_path = "/test_create_task"_pv;
        iris::println("Loading initial userspace task: {}"_sv, init_path);
        *iris::load_executable(*task4, init_path);
        scheduler.schedule_task(*task4);
    }

    iris::println("Starting the kernel scheduler..."_sv);

    scheduler.start();
}
}
