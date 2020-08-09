#define __is_libc
#include <fcntl.h>
#include <stdalign.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/syscall.h>

#include "loader.h"

#include "../libc/search/queue.c"
#include "../libc/string/memcpy.c"
#include "../libc/string/memmove.c"
#include "../libc/string/memset.c"
#include "../libc/string/strcmp.c"
#include "../libc/string/strcpy.c"
#include "../libc/string/strlen.c"

#define ONLY_DPRINTF
#define __is_loader
#include "../libc/stdio/printf.c"

void _exit(int status) {
    syscall(SC_EXIT, status);
    __builtin_unreachable();
}

ssize_t write(int fd, const void *buffer, size_t len) {
    return (ssize_t) syscall(SC_WRITE, fd, buffer, len);
}

int open(const char *path, int flags, ...) {
    va_list parameters;
    va_start(parameters, flags);
    int ret;
    mode_t mode = 0;
    if (flags & O_CREAT) {
        mode = va_arg(parameters, mode_t);
    }
    ret = (int) syscall(SC_OPENAT, AT_FDCWD, path, flags, mode);
    va_end(parameters);
    return ret;
}

int fstat(int fd, struct stat *st) {
    return (int) syscall(SC_FSTATAT, fd, "", st, AT_EMPTY_PATH);
}

int close(int fd) {
    return (int) syscall(SC_CLOSE, fd);
}

void *mmap(void *addr, size_t size, int prot, int flags, int fd, off_t offset) {
    return (void *) syscall(SC_MMAP, addr, size, prot, flags, fd, offset);
}

int mprotect(void *base, size_t size, int prot) {
    return (int) syscall(SC_MPROTECT, base, size, prot);
}

int munmap(void *base, size_t size) {
    return (int) syscall(SC_MUNMAP, base, size);
}

LOADER_PRIVATE void *fake_heap_end = NULL;

void *loader_malloc(size_t n) {
    if (ran_program) {
        return malloc(n);
    }

    n = ALIGN_UP(n, alignof(size_t));
    if (!fake_heap_end) {
    realloc_fake_heap:
        fake_heap_end = mmap(NULL, ALIGN_UP(n, PAGE_SIZE), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    alloc:
        *((size_t *) fake_heap_end) = n;
        fake_heap_end += n + sizeof(size_t);
        return fake_heap_end - n;
    }

    void *new_heap_end = fake_heap_end + n + sizeof(size_t);
    if ((uintptr_t) new_heap_end / PAGE_SIZE != (uintptr_t) fake_heap_end / PAGE_SIZE) {
        goto realloc_fake_heap;
    }
    goto alloc;
}

void *loader_calloc(size_t n, size_t m) {
    if (ran_program) {
        return calloc(n, m);
    }

    // NOTE: loader_malloc() memory is always zeroed since it comes from mmap() and can never be freed.
    return loader_malloc(n * m);
}

void *loader_realloc(void *p, size_t n) {
    if (ran_program) {
        return realloc(p, n);
    }

    if (!p) {
        return loader_malloc(n);
    }

    void *m = loader_malloc(n);
    size_t old_size = *(((size_t *) p) - 1);
    memcpy(m, p, MIN(old_size, n));
    loader_free(p);
    return m;
}

void loader_free(void *p) {
    if (ran_program) {
        free(p);
        return;
    }

    // No-op
    (void) p;
}
