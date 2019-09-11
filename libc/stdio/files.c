#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/types.h>

#define STDIO_OWNED 0x800000

FILE *stdio;
FILE *stdin;
FILE *stderr;

static FILE files[3];

FILE *fopen(const char *__restrict path, const char *__restrict mode) {
    int flags = 0;
    switch(mode[0]) {
        case 'r': 
            switch(mode[1]) {
                case '+':
                    flags = O_RDWR;
                    break;
                default:
                    flags = O_RDONLY;
                    break;
            }
            break;
        case 'w':
            switch(mode[1]) {
                case '+':
                    flags = O_RDWR | O_CREAT | O_TRUNC;
                    break;
                default:
                    flags = O_WRONLY | O_CREAT | O_TRUNC;
                    break;
            }
            break;
        case 'a':
            switch(mode[1]) {
                case '+':
                    flags = O_RDWR | O_CREAT | O_APPEND;
                    break;
                default:
                    flags = O_WRONLY | O_CREAT | O_APPEND;
                    break;
            }
            break;
    }

    mode_t __mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    int fd = open(path, flags, __mode);

    FILE *file = malloc(sizeof(FILE));
    file->pos = 0;
    file->buffer = malloc(BUFSIZ);
    file->fd = fd;
    file->length = BUFSIZ;
    file->flags = flags | STDIO_OWNED;
    file->buf_type = _IOFBF;
    file->eof = 0;
    file->error = 0;

    return file;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    ssize_t ret = read(stream->fd, ptr, nmemb * size);

    if (ret < 0) {
        return 0;
    }

    return (size_t) ret;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    ssize_t ret = write(stream->fd, ptr, nmemb * size);

    if (ret < 0) {
        return 0;
    }

    return (size_t) ret;
}

int fclose(FILE *stream) {
    /* Should flush buffer once streams have a buffer */

    int ret = close(stream->fd);
    if (ret < 0) {
        return EOF;
    }

    if (stream->buffer != NULL && stream->flags & STDIO_OWNED) {
        free(stream->buffer);
    }
    free(stream);
    return 0;
}

void init_files() {
    /* stdin */
    files[0].fd = 0;
    files[0].pos = 0;
    files[0].buf_type = _IOLBF;
    files[0].buffer = malloc(BUFSIZ);
    files[0].length = BUFSIZ;
    files[0].eof = 0;
    files[0].error = 0;
    files[0].flags = O_RDWR | STDIO_OWNED;
    stdin = files + 0;

    /* stdio */
    files[1].fd = 1;
    files[1].buf_type = _IOLBF;
    files[1].buffer = malloc(BUFSIZ);
    files[1].length = BUFSIZ;
    files[1].eof = 0;
    files[1].error = 0;
    files[1].flags = O_RDWR | STDIO_OWNED;
    files[1].pos = 0;
    stdio = files + 1;

    /* stderr */ 
    files[2].fd = 2;
    files[2].pos = 0;
    files[2].buf_type = _IONBF | STDIO_OWNED;
    files[2].buffer = NULL;
    files[2].length = 0;
    files[2].eof = 0;
    files[2].error = 0;
    files[2].flags = O_RDWR;
    stderr = files + 2;
}