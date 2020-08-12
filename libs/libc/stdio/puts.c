#include <bits/lock.h>
#include <stdio.h>
#include <string.h>
#include <sys/uio.h>

int puts(const char *s) {
    int ret = 1;
    size_t len = strlen(s);

    __lock_recursive(&stdout->__lock);

    if (stdout->__flags & __STDIO_ERROR) {
        return EOF;
    }

    if (stdout->__flags & __STDIO_LAST_OP_READ) {
        if (fflush_unlocked(stdout)) {
            return EOF;
        }

        stdout->__flags &= ~__STDIO_LAST_OP_READ;
    }

    stdout->__flags |= __STDIO_LAST_OP_WRITE;
    stdout->__flags &= ~__STDIO_EOF;

    if ((stdout->__flags & _IONBF) || ((stdout->__flags & _IOLBF) && stdout->__buffer_length == 0)) {
        char newline = '\n';
        struct iovec vec[2] = { { .iov_base = (char *) s, .iov_len = len }, { .iov_base = &newline, .iov_len = 1 } };
        if (writev(stdout->__fd, vec, 2) != (ssize_t)(len + 1)) {
            ret = EOF;
        }

        goto finish_puts;
    }

    if (stdout->__flags & _IOLBF) {
        char newline = '\n';
        struct iovec vec[3] = { { .iov_base = stdout->__buffer, .iov_len = stdout->__buffer_length },
                                { .iov_base = (char *) s, .iov_len = len },
                                { .iov_base = &newline, .iov_len = 1 } };
        if (writev(stdout->__fd, vec, 3) != (ssize_t)(stdout->__buffer_length + len + 1)) {
            ret = EOF;
            stdout->__flags |= __STDIO_ERROR;
        }

        stdout->__position = stdout->__buffer_length = 0;
        goto finish_puts;
    }

    if (fwrite_unlocked(s, sizeof(char), len, stdout) != len) {
        ret = EOF;
        goto finish_puts;
    }

    ret = fputc_unlocked('\n', stdout);

finish_puts:
    __unlock_recursive(&stdout->__lock);
    return ret;
}
