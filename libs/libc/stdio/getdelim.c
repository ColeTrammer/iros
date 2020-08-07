#include <bits/lock.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_LINE_BUFFER_SIZE 100

ssize_t getdelim(char **__restrict line_ptr, size_t *__restrict n, int delim, FILE *__restrict stream) {
    if (*line_ptr == NULL) {
        if (*n == 0) {
            *n = DEFAULT_LINE_BUFFER_SIZE;
        }

        *line_ptr = malloc(*n);
    }

    __lock(&stream->__lock);

    size_t pos = 0;
    for (;;) {
        int c = fgetc_unlocked(stream);

        /* Indicate IO error or out of lines */
        if (c == EOF && (ferror_unlocked(stream) || pos == 0)) {
            __unlock(&stream->__lock);
            return -1;
        }

        if (c == EOF) {
            (*line_ptr)[pos] = '\0';
            break;
        }

        if (c == delim) {
            (*line_ptr)[pos++] = c;
            (*line_ptr)[pos] = '\0';
            break;
        }

        (*line_ptr)[pos++] = c;

        if (pos + 1 >= *n) {
            *n *= 2;
            *line_ptr = realloc(*line_ptr, *n);
            if (!*line_ptr) {
                __unlock(&stream->__lock);
                return -1;
            }
        }
    }

    __unlock(&stream->__lock);

    return (ssize_t) pos;
}
