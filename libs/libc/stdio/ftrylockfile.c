#include <bits/lock.h>
#include <stdio.h>

int ftrylockfile(FILE *stream) {
    return !__trylock_recursive(&stream->__lock);
}
