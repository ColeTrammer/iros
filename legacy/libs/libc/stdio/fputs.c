#include <bits/lock.h>
#include <stdio.h>

int fputs(const char *__restrict s, FILE *__restrict stream) {
    __lock_recursive(&stream->__lock);
    int ret = fputs_unlocked(s, stream);
    __unlock_recursive(&stream->__lock);
    return ret;
}
