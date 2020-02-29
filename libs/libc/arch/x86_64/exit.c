#define __libc_internal

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern void (*__fini_array_start[])(void) __attribute__((visibility("hidden")));
extern void (*__fini_array_end[])(void) __attribute__((visibility("hidden")));

extern void _fini(void);

__attribute__((__noreturn__)) void exit(int status) {
    __on_exit();

    ssize_t count = (ssize_t)(__fini_array_end - __fini_array_start);
    for (ssize_t i = count - 1; i >= 0; i--) {
        __fini_array_start[i]();
    }

    _fini();

    fflush_unlocked(NULL);

    _exit(status);

    __builtin_unreachable();
}