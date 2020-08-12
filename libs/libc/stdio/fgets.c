#include <bits/lock.h>
#include <stdio.h>

char *fgets(char *__restrict s, int size, FILE *__restrict stream) {
    __lock_recursive(&stream->__lock);
    char *ret = fgets_unlocked(s, size, stream);
    __unlock_recursive(&stream->__lock);
    return ret;
}
