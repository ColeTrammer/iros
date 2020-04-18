#ifndef OLD_STDIO

#include <bits/lock.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

long ftell(FILE *stream) {
    __lock(&stream->__lock);
    off_t ret = stream->__position;

    // NOTE: this is has very strange consequences when the actual file position is 0
    if (stream->__flags & __STDIO_HAS_UNGETC_CHARACTER) {
        ret--;
    }

    off_t res = lseek(stream->__fd, 0, SEEK_CUR);
    if (res == -1) {
        ret = -1;
    } else if (res == 0) {
        ret = MAX(0, res + ret);
    } else {
        ret += res;
    }

    __unlock(&stream->__lock);
    return ret;
}

#endif /* OLD_STDIO */