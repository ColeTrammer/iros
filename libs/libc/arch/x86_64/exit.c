#define __libc_internal

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void __cxa_atexit() {}

__attribute__((__noreturn__)) void exit(int status) {
    __on_exit();

    fflush_unlocked(NULL);

    _exit(status);

    __builtin_unreachable();
}