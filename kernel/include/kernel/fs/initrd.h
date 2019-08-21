#ifndef _KERNEL_FS_INITRD_H
#define _KERNEL_FS_INITRD_H 1

#include <stdint.h>

#include <kernel/fs/file_system.h>

#define INITRD_MAX_FILE_NAME_LENGTH 64

struct initrd_file_entry {
    char name[INITRD_MAX_FILE_NAME_LENGTH];
    uint32_t offset;
    uint32_t length;
} __attribute__((packed));

void init_initrd();

#endif /* _KERNEL_FS_INITRD_H */