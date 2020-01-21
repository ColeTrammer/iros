#ifdef NEW_STDIO

#include <bits/lock.h>
#include <stdio.h>

void rewind(FILE *stream) {
    fseek(stream, 0, SEEK_SET);
    clearerr(stream);
}

#endif /* NEW_STDIO */