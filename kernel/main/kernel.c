#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/fs/dev.h>
#include <kernel/fs/vfs.h>
#include <kernel/hal/hal.h>
#include <kernel/hal/output.h>
#include <kernel/irqs/handlers.h>
#include <kernel/mem/page_frame_allocator.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/net/net.h>
#include <kernel/proc/elf64.h>
#include <kernel/proc/task.h>
#include <kernel/sched/task_sched.h>
#include <kernel/time/clock.h>
#include <kernel/time/timer.h>

void kernel_main(uintptr_t kernel_phys_start, uintptr_t kernel_phys_end, uintptr_t inintrd_phys_start, uint64_t initrd_phys_end,
                 uint32_t *multiboot_info) {
    init_hal();
    init_irq_handlers();
    init_page_frame_allocator(kernel_phys_start, kernel_phys_end, inintrd_phys_start, initrd_phys_end, multiboot_info);
    init_kernel_task();
    init_vm_allocator(inintrd_phys_start, initrd_phys_end);
    init_vfs();
    init_drivers();
    init_clocks();
    init_timers();
    init_task_sched();
    init_net();

    /* Mount hdd0 at / */
    struct device *hdd0 = dev_get_device(0x430);
    assert(hdd0);
    int error = 0;
    error = fs_mount(hdd0, "/", "ext2");
    assert(error == 0);

    // Mount tmpfs at /dev/shm
    error = fs_mount(NULL, "/dev/shm", "tmpfs");
    assert(error == 0);

    // NOTE: if we put these symbols on the initrd instead of in /boot/os_2.o, thse symbols
    //       could be loaded sooner
    init_kernel_symbols();

    // Start Shell
    struct task *shell = load_task("/bin/start");
    sched_add_task(shell);

    sched_run_next();

    while (1)
        ;
}
