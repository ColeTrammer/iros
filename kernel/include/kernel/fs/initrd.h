#ifndef _KERNEL_FS_INITRD_H
#define _KERNEL_FS_INITRD_H 1

#include <stdint.h>

#include <kernel/fs/file_system.h>
#include <kernel/fs/tnode.h>

#define INITRD_MAX_FILE_NAME_LENGTH 64

struct initrd_file_entry {
    char name[INITRD_MAX_FILE_NAME_LENGTH];
    uint32_t offset;
    uint32_t length;
} __attribute__((packed));

void init_initrd();

struct tnode *initrd_lookup(struct inode *inode, const char *name);
struct file *initrd_open(struct inode *inode, int *error);
ssize_t initrd_read(struct file *file, void *buffer, size_t len);
struct tnode *initrd_mount(struct file_system *fs, char *device_path);

#endif /* _KERNEL_FS_INITRD_H */