#include <stdio.h>
#include <stdlib.h>

__attribute__((__noreturn__))
void abort() {
    while (1);
}