#include <sys/param.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <kernel/fs/fs_manager.h>
#include <kernel/fs/initrd.h>
#include <kernel/fs/file_system.h>
#include <kernel/mem/vm_region.h>
#include <kernel/mem/vm_allocator.h>

static struct file_system **file_systems;

void init_fs_manager() {
    file_systems = calloc(MAX_FILE_SYSTEMS, sizeof(struct file_system*));

    init_initrd();
}

VFILE *fs_open(const char *file_name) {
    if (file_name == NULL) {
        return NULL;
    }

    int fs_index = file_name[0] - 'A';
    if (fs_index < 0 || fs_index >= MAX_FILE_SYSTEMS) {
        return NULL;
    }

    if (file_name[1] != ':') {
        return NULL;
    }

    if (file_systems[fs_index] == NULL) {
        return NULL;
    }

    return file_systems[fs_index]->open(file_name + 2);
}

void fs_close(VFILE *file) {
    file_systems[file->device]->close(file);
}

void fs_read(VFILE *file, void *buffer, size_t len) {
    file_systems[file->device]->read(file, buffer, MIN(len, file->length - (file->position - file->start)));
}

void fs_write(VFILE *file, const void *buffer, size_t len) {
    file_systems[file->device]->write(file, buffer, len);
}

int fs_seek(VFILE *file, long offset, int whence) {
    long new_position;
    if (whence == SEEK_SET) {
        new_position = offset;
    } else if (whence == SEEK_CUR) {
        new_position = file->position + offset;
    } else if (whence == SEEK_END) {
        new_position = file->length + offset;
    } else {
        printf("Invalid arguments for fs_seek - whence: %d\n", whence);
        abort();
    }

    if (new_position > file->length) {
        return -1;
    }

    file->position = new_position;
    return 0;
}

long fs_tell(VFILE *file) {
    return file->position;
}

void load_fs(struct file_system *file_system, int device_id) {
    if (device_id < 0 || device_id >= MAX_FILE_SYSTEMS) {
        printf("Error invalid device id: %d\n", device_id);
        abort();
    }

    file_systems[device_id] = file_system;
    file_systems[device_id]->mount();
}

void unload_fs(int device_id) {
    if (device_id < 0 || device_id >= MAX_FILE_SYSTEMS) {
        printf("Error invalid device id: %d\n", device_id);
        abort();
    }

    free(file_systems[device_id]);
    file_systems[device_id] = NULL;
}