#ifdef NEW_STDIO

#include <bits/lock.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

long ftell(FILE *stream) {
    __lock(&stream->__lock);
    off_t ret = 0;

    // NOTE: this is has very strange consequences when the actual file position is 0
    if (stream->__flags & __STDIO_HAS_UNGETC_CHARACTER) {
        ret--;
    }

    off_t res = lseek(stream->__fd, SEEK_CUR, 0);
    if (res == -1) {
        ret = -1;
    } else if (res == 0) {
        ret = 0;
    } else {
        ret += res;
    }

    __unlock(&stream->__lock);
    return ret;
}

#endif /* NEW_STDIO */