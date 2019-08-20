#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <kernel/fs/fs_manager.h>
#include <kernel/fs/initrd.h>
#include <kernel/fs/file_system.h>
#include <kernel/mem/vm_region.h>
#include <kernel/mem/vm_allocator.h>

static struct file_system *file_systems;

void init_fs_manager() {
    file_systems = calloc(MAX_FILE_SYSTEMS, sizeof(struct file_system));

    init_initrd(file_systems + FS_INITRD_INDEX);
    file_systems[FS_INITRD_INDEX].mount();
}