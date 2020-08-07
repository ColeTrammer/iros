#include <bits/lock.h>
#include <stdio.h>

char *fgets(char *__restrict s, int size, FILE *__restrict stream) {
    __lock(&stream->__lock);
    char *ret = fgets_unlocked(s, size, stream);
    __unlock(&stream->__lock);
    return ret;
}
