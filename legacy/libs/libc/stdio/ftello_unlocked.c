#include <stdio.h>
#include <unistd.h>

off_t ftello_unlocked(FILE *stream) {
    off_t ret = 0;
    if (stream->__flags & __STDIO_LAST_OP_WRITE) {
        off_t res = lseek(stream->__fd, 0, SEEK_CUR);
        if (res == -1) {
            return -1;
        }

        return res + stream->__position;
    }

    if (stream->__flags & __STDIO_LAST_OP_READ) {
        // NOTE: this is has very strange consequences when the actual file position is 0
        if (stream->__flags & __STDIO_HAS_UNGETC_CHARACTER) {
            ret--;
        }

        off_t res = lseek(stream->__fd, 0, SEEK_CUR);
        if (res == -1) {
            return -1;
        } else if (res == 0) {
            return 0;
        } else {
            return ret + res - stream->__buffer_length + stream->__position;
        }
    }

    return lseek(stream->__fd, 0, SEEK_CUR);
}
