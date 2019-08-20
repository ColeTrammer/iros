#ifndef _KERNEL_FS_FILE_SYSTEM_H
#define _KERNEL_FS_FILE_SYSTEM_H 1

#include <stddef.h>

#include "vfile.h"

struct file_system {
    char name[8];
    
    VFILE *(*open) (const char *filename);
    void (*close) (VFILE *file);

    void (*read) (VFILE *file, void *buffer, size_t len);
    void (*write) (VFILE *file, const void *buffer, size_t len);

    void (*mount) ();
};

#endif /* _KERNEL_FS_FILE_SYSTEM_H */