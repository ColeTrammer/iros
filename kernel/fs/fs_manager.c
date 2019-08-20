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

VFILE *fs_open(const char *file_name) {
    if (file_name == NULL) {
        return NULL;
    }

    char fs_index = file_name[0] - 'A';
    if (fs_index < 0 || fs_index >= MAX_FILE_SYSTEMS) {
        return NULL;
    }

    if (file_name[1] != ":") {
        return NULL;
    }

    if (file_systems[fs_index].name == NULL) {
        return NULL;
    }

    return file_systems[fs_index].open(file_name);
}

void fs_close(VFILE *file) {
    file_systems[file->device].close(file);
}

void fs_read(VFILE *file, void *buffer, size_t len) {
    file_systems[file->device].read(file, buffer, len);
}

void fs_write(VFILE *file, const void *buffer, size_t len) {
    file_systems[file->device].write(file, buffer, len);
}