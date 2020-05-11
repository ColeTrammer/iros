#ifndef OLD_STDIO

#include <bits/lock.h>
#include <stdio.h>

int fsetpos(FILE *stream, const fpos_t *pos) {
    return fseek(stream, *pos, SEEK_SET);
}

#endif /* OLD_STDIO */
