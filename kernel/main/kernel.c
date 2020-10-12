#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/fs/dev.h>
#include <kernel/fs/disk_sync.h>
#include <kernel/fs/vfs.h>
#include <kernel/hal/hal.h>
#include <kernel/hal/output.h>
#include <kernel/irqs/handlers.h>
#include <kernel/mem/page_frame_allocator.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/net/interface.h>
#include <kernel/proc/elf64.h>
#include <kernel/proc/task.h>
#include <kernel/proc/task_finalizer.h>
#include <kernel/sched/task_sched.h>
#include <kernel/util/init.h>
#include <kernel/util/list.h>

void kernel_main(uint32_t *multiboot_info) {
    init_hal();
    init_irq_handlers();
    init_page_frame_allocator(multiboot_info);
    init_kernel_process();
    init_vm_allocator();
    init_cpus();

    enable_interrupts();
    INIT_DO_LEVEL(fs);

    /* Mount INITRD as root */
    int error = fs_mount(NULL, "/", "initrd");
    assert(error == 0);

    INIT_DO_LEVEL(driver);
    enumerate_devices();
    INIT_DO_LEVEL(time);

    init_task_sched();
    init_task_finalizer();
    INIT_DO_LEVEL(net);
    init_disk_sync_task();

    /* Mount sda1 at / */
    struct fs_device *sda1 = dev_get_device(0x00501);
    assert(sda1);
    error = 0;
    error = fs_mount(sda1, "/", "ext2");
    assert(error == 0);
    dev_drop_device(sda1);

    // NOTE: if we put these symbols on the initrd instead of in /boot/os_2.o, thse symbols
    //       could be loaded sooner
    init_kernel_symbols();
    init_smp();

    // Mount procfs at /proc
    error = fs_mount(NULL, "/proc", "procfs");
    assert(error == 0);

    // Mount tmpfs at /tmp
    error = fs_mount(NULL, "/tmp", "tmpfs");
    assert(error == 0);

    // Mount tmpfs at /dev/shm
    error = fs_mount(NULL, "/dev/shm", "tmpfs");
    assert(error == 0);
    start_userland();
}
