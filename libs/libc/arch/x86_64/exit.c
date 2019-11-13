#define __libc_internal

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

void __cxa_atexit() {}

__attribute__((__noreturn__))
void _Exit(int status) {
    _exit(status);

    __builtin_unreachable();
}

__attribute__((__noreturn__))
void exit(int status) {
    __on_exit();

    fflush(stdin);
    fflush(stdout);
    fflush(stderr);

    _Exit(status);

    __builtin_unreachable();
}