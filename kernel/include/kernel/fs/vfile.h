#ifndef _KERNEL_FS_VFILE_H
#define _KERNEL_FS_VFILE_H 1

#include <stdint.h>

#define FS_FILE 0
#define FS_DIRECTORY 1

typedef struct _VFILE {
    char name[255];
    uint32_t length;
    uint32_t flags;
    uint32_t eof;
    uint32_t position;
    uint32_t cluster;
    uint32_t device;
} VFILE;

#endif /* _KERNEL_FS_VFILE_H */