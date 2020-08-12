#include <bits/lock.h>
#include <stdio.h>

void funlockfile(FILE *stream) {
    __unlock_recursive(&stream->__lock);
}
