#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

__attribute__((__noreturn__))
void _Exit(int status) {
    _exit(status);

    __builtin_unreachable();
}

__attribute__((__noreturn__))
void exit(int status) {
    /* Should Call atexit Functions */

    fflush(stdin);
    fflush(stdio);
    fflush(stderr);

    _Exit(status);

    __builtin_unreachable();
}