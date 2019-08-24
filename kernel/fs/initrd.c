#include <stdio.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/fs/vfile.h>
#include <kernel/fs/initrd.h>
#include <kernel/fs/file_system.h>
#include <kernel/fs/fs_manager.h>
#include <kernel/mem/vm_region.h>
#include <kernel/mem/vm_allocator.h>

static int64_t num_files;
static uintptr_t initrd_start;
static struct initrd_file_entry *file_list;

VFILE *initrd_open(const char *file_name) {
    if (strlen(file_name) > INITRD_MAX_FILE_NAME_LENGTH) {
        return NULL;
    }

    struct initrd_file_entry *entry = file_list;
    for (int64_t i = 0; i < num_files; i++) {
        if (strcmp(file_name, entry[i].name) == 0) {
            VFILE *file = calloc(sizeof(VFILE), 1);
            strcpy(file->name, file_name);
            file->length = entry[i].length;
            file->flags = FS_FILE;
            file->position = entry[i].offset;
            file->device = FS_INITRD_INDEX;
            return file;
        }
    }
    
    return NULL;
}

void initrd_close(VFILE *file) {
    free(file);
}

void initrd_read(VFILE *file, void *buffer, size_t len) {
    memcpy(buffer, (void*) (initrd_start + file->position), len);
}

void initrd_write(VFILE *file, const void *buffer, size_t len) {
    initrd_close(file);
    printf("Can't write to initrd.\nBuffer: %#.16lX | Len: %u\n", buffer, len);
}

void initrd_mount() {
    struct vm_region *initrd = find_vm_region(VM_INITRD);
    
    initrd_start = initrd->start;
    num_files = *((int64_t*) initrd_start);
    file_list = (struct initrd_file_entry*) (initrd_start + sizeof(int64_t));
}

void init_initrd() {
    struct file_system *fs = malloc(sizeof(struct file_system));
    fs->name = "initrd";
    fs->open = &initrd_open;
    fs->close = &initrd_close;
    fs->read = &initrd_read;
    fs->write = &initrd_write;
    fs->mount = &initrd_mount;

    load_fs(fs, FS_INITRD_INDEX);
}