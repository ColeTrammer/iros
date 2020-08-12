#include <bits/lock.h>
#include <stdio.h>

void flockfile(FILE *stream) {
    __lock_recursive(&stream->__lock);
}
