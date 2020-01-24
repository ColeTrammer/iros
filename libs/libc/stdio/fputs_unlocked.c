#ifndef OLD_STDIO

#include <stdio.h>
#include <string.h>

int fputs_unlocked(const char *__restrict s, FILE *__restrict stream) {
    size_t len = strlen(s);
    size_t ret = fwrite_unlocked(s, sizeof(char), len, stream);
    return ret == len ? 1 : EOF;
}

#endif /* OLD_STDIO */