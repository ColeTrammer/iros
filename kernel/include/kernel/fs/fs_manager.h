#ifndef _KERNEL_FS_FS_MANAGER_H
#define _KERNEL_FS_FS_MANAGER_H 1

#include <stddef.h>

#include "vfile.h"
#include "file_system.h"

#define MAX_FILE_SYSTEMS 27
#define FS_INITRD_INDEX (MAX_FILE_SYSTEMS - 1)

void init_fs_manager();

VFILE *fs_open(const char *filename);
void fs_close(VFILE *file);

void fs_read(VFILE *file, void *buffer, size_t len);
void fs_write(VFILE *file, const void *buffer, size_t len);

int fs_seek(VFILE *file, long offset, int whence);
long fs_tell(VFILE *file);

void load_fs(struct file_system *file_system, int device_id);
void unload_fs(int device_id);

#endif /* _KERNEL_FS_FS_MANAGER_H */