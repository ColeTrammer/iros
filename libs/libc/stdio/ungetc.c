#include <bits/lock.h>
#include <stdio.h>

int ungetc(int c, FILE *stream) {
    int ret = c;
    __lock_recursive(&stream->__lock);

    __stdio_log(stream, "ungetc: %d %c", stream->__fd, c);

    if (stream->__flags & __STDIO_HAS_UNGETC_CHARACTER) {
        ret = EOF;
    } else {
        stream->__flags |= __STDIO_HAS_UNGETC_CHARACTER;
        stream->__flags &= ~__STDIO_EOF;
        stream->__ungetc_character = c;
    }
    __unlock_recursive(&stream->__lock);
    return ret;
}
