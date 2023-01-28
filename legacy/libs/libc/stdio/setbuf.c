#include <stdio.h>

void setbuf(FILE *__restrict stream, char *__restrict buf) {
    setvbuf(stream, buf, buf ? _IOFBF : _IONBF, BUFSIZ);
}
