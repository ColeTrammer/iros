#ifndef OLD_STDIO

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static FILE files[3];

FILE *stdin = &files[0];
FILE *stdout = &files[1];
FILE *stderr = &files[2];

FILE *__file_list_head = files + 0;
FILE *__file_list_tail = files + 2;
unsigned int __file_list_lock;

static char static_file_buffer[2 * BUFSIZ];

#ifndef NDEBUG
int __should_log;
#endif /* NDEBUG */

void init_files(int isatty_mask) {
    /* stdin */
    stdin->__fd = 0;
    stdin->__position = 0;
    stdin->__flags |= (isatty_mask & (1 << 0)) ? _IONBF : _IOFBF;
    stdin->__buffer = static_file_buffer;
    stdin->__buffer_length = 0;
    stdin->__buffer_max = BUFSIZ;
    stdin->__flags |= __STDIO_READABLE | __STDIO_WRITABLE;
    stdin->__prev = NULL;
    stdin->__next = stdout;

    /* stdout */
    stdout->__fd = 1;
    stdout->__flags |= (isatty_mask & (1 << 1)) ? _IOLBF : _IOFBF;
    stdout->__buffer = static_file_buffer + BUFSIZ;
    stdout->__buffer_max = BUFSIZ;
    stdout->__buffer_length = 0;
    stdout->__flags |= __STDIO_READABLE | __STDIO_WRITABLE;
    stdout->__position = 0;
    stdout->__prev = stdin;
    stdout->__next = stderr;

    /* stderr */
    stderr->__fd = 2;
    stderr->__flags |= _IONBF;
    stderr->__position = 0;
    stderr->__buffer = NULL;
    stderr->__buffer_length = 0;
    stderr->__buffer_max = 0;
    stderr->__flags |= __STDIO_OWNED | __STDIO_READABLE | __STDIO_WRITABLE;
    stderr->__prev = stdout;
    stderr->__next = NULL;

#ifndef NDEBUG
    __should_log = getenv("STDIO_DEBUG") ? 1 : 0;
#endif /* NDEBUG */
}

#endif /* OLD_STDIO */
