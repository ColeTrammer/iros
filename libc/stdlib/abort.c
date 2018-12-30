#include <stdio.h>
#include <stdlib.h>

__attribute__((__noreturn__))
void abort() {
    printf("%s\n", "abort");
    while (1);
    __builtin_unreachable();
}