#ifndef _KERNEL_FS_FS_MANAGER_H
#define _KERNEL_FS_FS_MANAGER_H 1

#include <stddef.h>

#include "vfile.h"
#include "file_system.h"

#define MAX_FILE_SYSTEMS 27
#define FS_INITRD_INDEX (MAX_FILE_SYSTEMS - 1)

void init_fs_manager();

VFILE *open_file(const char *filename);
void close_file(VFILE *file);

void read_file(VFILE *file, void *buffer, size_t len);
void write_file(VFILE *file, const void *buffer, size_t len);

void load_fs(struct file_system *file_system, char device_id);
void unload_fs(struct file_system *file_system);

#endif /* _KERNEL_FS_FS_MANAGER_H */