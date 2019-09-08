#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/fs/fs_manager.h>
#include <kernel/irqs/handlers.h>
#include <kernel/mem/page_frame_allocator.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/process.h>
#include <kernel/sched/process_sched.h>
#include <kernel/hal/hal.h>
#include <kernel/hal/output.h>

void kernel_main(uintptr_t kernel_phys_start, uintptr_t kernel_phys_end, uintptr_t inintrd_phys_start, uint64_t initrd_phys_end, uint32_t *multiboot_info) {
    init_hal();
    init_irq_handlers();
    init_page_frame_allocator(kernel_phys_start, kernel_phys_end, inintrd_phys_start, initrd_phys_end, multiboot_info);
    init_kernel_process();
    init_vm_allocator(inintrd_phys_start, initrd_phys_end);
    init_drivers();
    init_fs_manager();
    init_process_sched();

    // Test Programs
    struct process *test = load_process("[:test.o");
    sched_add_process(test);

    sched_run_next();

    while (1);
}