#include <bits/lock.h>
#include <stdio.h>

size_t fread(void *__restrict buf, size_t size, size_t nmemb, FILE *__restrict stream) {
    __lock(&stream->__lock);
    size_t ret = fread_unlocked(buf, size, nmemb, stream);
    __unlock(&stream->__lock);
    return ret;
}
