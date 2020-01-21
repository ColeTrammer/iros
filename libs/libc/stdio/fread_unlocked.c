#ifdef NEW_STDIO

#include <stdio.h>

size_t fread_unlocked(void *__restrict buf, size_t size, size_t nmemb, FILE *__restrict stream) {
    size_t bytes_read = 0;
    size_t to_read = size * nmemb;
    for (; bytes_read < to_read;) {
        int c = fgetc_unlocked(stream);
        if (c == EOF) {
            break;
        }

        ((unsigned char *) buf)[bytes_read++] = (unsigned char) c;
    }

    return bytes_read / size;
}

#endif /* NEW_STDIO */