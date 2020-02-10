#ifndef OLD_STDIO

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

FILE *stdout;
FILE *stdin;
FILE *stderr;

static FILE files[3];

FILE *__file_list_head = files + 0;
FILE *__file_list_tail = files + 2;
unsigned int __file_list_lock;

void init_files(int isatty_mask) {
    /* stdin */
    files[0].__fd = 0;
    files[0].__position = 0;
    files[0].__flags |= (isatty_mask & (1 << 0)) ? _IONBF : _IOFBF;
    files[0].__buffer = malloc(BUFSIZ);
    files[0].__buffer_length = 0;
    files[0].__buffer_max = BUFSIZ;
    files[0].__flags |= __STDIO_OWNED | __STDIO_READABLE | __STDIO_WRITABLE;
    files[0].__prev = NULL;
    files[0].__next = files + 1;
    stdin = files + 0;

    /* stdout */
    files[1].__fd = 1;
    files[1].__flags |= (isatty_mask & (1 << 1)) ? _IOLBF : _IOFBF;
    files[1].__buffer = malloc(BUFSIZ);
    files[1].__buffer_max = BUFSIZ;
    files[1].__buffer_length = 0;
    files[1].__flags |= __STDIO_OWNED | __STDIO_READABLE | __STDIO_WRITABLE;
    files[1].__position = 0;
    files[1].__prev = files + 0;
    files[1].__next = files + 2;
    stdout = files + 1;

    /* stderr */
    files[2].__fd = 2;
    files[2].__flags |= _IONBF;
    files[2].__position = 0;
    files[2].__buffer = NULL;
    files[2].__buffer_length = 0;
    files[2].__buffer_max = 0;
    files[2].__flags |= __STDIO_OWNED | __STDIO_READABLE | __STDIO_WRITABLE;
    files[2].__prev = files + 1;
    files[2].__next = NULL;
    stderr = files + 2;
}

#endif /* OLD_STDIO */