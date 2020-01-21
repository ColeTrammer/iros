#ifdef NEW_STDIO

#include <stdio.h>

int fputs_unlocked(const char *__restrict s, FILE *__restrict stream) {
    for (size_t i = 0; s[i] != '\0'; i++) {
        if (fputc(s[i], stream) == EOF) {
            return EOF;
        }
    }

    return 1;
}

#endif /* NEW_STDIO */