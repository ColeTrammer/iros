#include <bits/lock.h>
#include <stdio.h>
#include <unistd.h>

int fgetpos(FILE *__restrict stream, fpos_t *__restrict pos) {
    __lock(&stream->__lock);
    off_t ret = lseek(stream->__fd, 0, SEEK_CUR);
    __unlock(&stream->__lock);

    if (ret >= 0) {
        *pos = ret;
        return 0;
    }

    return -1;
}
