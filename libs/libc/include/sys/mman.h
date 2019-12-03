
#ifndef _SYS_MMAN_H
#define _SYS_MMAN_H 1

#include <bits/mode_t.h>
#include <bits/off_t.h>
#include <bits/size_t.h>

#define PROT_NONE  0
#define PROT_EXEC  1
#define PROT_READ  2
#define PROT_WRITE 4

#define MAP_SHARED  1
#define MAP_PRIVATE 2

#define MAP_FIXED     4
#define MAP_ANONYMOUS 8
#define MAP_ANON      MAP_ANONYMOUS
#define MAP_STACK     16

#define MAP_FAILED ((void *) -1)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void *addr, size_t length);

int shm_open(const char *name, int oflag, mode_t mode);
int shm_unlink(const char *name);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_MMAN_H */