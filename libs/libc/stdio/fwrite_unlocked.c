#ifdef NEW_STDIO

#include <bits/lock.h>
#include <stdio.h>

size_t fwrite_unlocked(const void *__restrict buf, size_t size, size_t nmemb, FILE *__restrict stream) {
    size_t bytes_written = 0;
    size_t to_write = size * nmemb;
    for (; bytes_written < to_write; bytes_written++) {
        if (fputc_unlocked(((const unsigned char *) buf)[bytes_written], stream) == EOF) {
            break;
        }
    }

    return bytes_written / size;
}

#endif /* NEW_STDIO */