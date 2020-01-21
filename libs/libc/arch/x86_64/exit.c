#define __libc_internal

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void __cxa_atexit() {}

__attribute__((__noreturn__)) void _Exit(int status) {
    _exit(status);

    __builtin_unreachable();
}

__attribute__((__noreturn__)) void exit(int status) {
    __on_exit();

    fflush(NULL);

    _Exit(status);

    __builtin_unreachable();
}