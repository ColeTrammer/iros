#include <bits/lock.h>
#include <stdio.h>

size_t fwrite(const void *__restrict ptr, size_t size, size_t nmemb, FILE *__restrict stream) {
    __lock(&stream->__lock);
    size_t ret = fwrite_unlocked(ptr, size, nmemb, stream);
    __unlock(&stream->__lock);
    return ret;
}
