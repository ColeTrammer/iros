#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

__attribute__((__noreturn__))
void abort() {
#ifdef __is_libk
    while (1);
#else
    // Should raise SIGABRT instead
    _Exit(1);
#endif /* __is_libk */
}