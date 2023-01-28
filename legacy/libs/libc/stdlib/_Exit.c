#include <stdlib.h>
#include <unistd.h>

__attribute__((__noreturn__)) void _Exit(int status) {
    _exit(status);

    __builtin_unreachable();
}
