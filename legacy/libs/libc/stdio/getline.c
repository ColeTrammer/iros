#include <stdio.h>

ssize_t getline(char **__restrict line_ptr, size_t *__restrict n, FILE *__restrict stream) {
    return getdelim(line_ptr, n, '\n', stream);
}
