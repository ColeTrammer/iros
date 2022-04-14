#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/boot/boot_info.h>
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

void kernel_main(struct multiboot2_info *multiboot_info) {
    init_hal();
    init_irq_handlers();
    init_boot_info_from_multiboot2(multiboot_info);
    init_page_frame_allocator(multiboot_info);
    init_kernel_process();
    init_vm_allocator();
    init_cpus();

    enable_interrupts();
    INIT_DO_LEVEL(fs);
    assert(!fs_mount_initrd());
    INIT_DO_LEVEL(driver);
    enumerate_devices();
    INIT_DO_LEVEL(time);

    init_task_sched();
    init_task_finalizer();
    INIT_DO_LEVEL(net);
    init_disk_sync_task();

    struct fs_root_desc root_desc = {
        .type = FS_ROOT_TYPE_FS_NAME,
        .fs_name = "ext2",
    };
    assert(!fs_mount_root(root_desc));

    // NOTE: if we put these symbols on the initrd instead of in /boot/iros.o, thse symbols
    //       could be loaded sooner
    init_kernel_symbols();
    init_smp();

    start_userland();
}
