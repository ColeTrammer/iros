#ifndef OLD_STDIO

#include <bits/lock.h>
#include <stdio.h>

char *fgets_unlocked(char *__restrict buf, int size, FILE *__restrict stream) {
    int i = 0;
    while (i < size - 1) {
        int c = fgetc_unlocked(stream);
        if (c == EOF) {
            if (stream->__flags & __STDIO_ERROR || i == 0) {
                return NULL;
            }
            break;
        }

        if (c == '\n') {
            buf[i++] = (char) c;
            break;
        }

        buf[i++] = (char) c;
    }

    buf[i] = '\0';
    return buf;
}

#endif /* OLD_STDIO */