#define __libc_internal

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

int __p_child_pid = 0;

FILE *popen(const char *command, const char *mode) {
    if (!command || !mode) {
        errno = EINVAL;
        return NULL;
    }

    int fds[2];
    if (pipe(fds)) {
        return NULL;
    }

    FILE *f;

    if (mode[0] == 'r') {
        f = fdopen(fds[0], mode);
    } else if (mode[0] == 'w') {
        f = fdopen(fds[1], mode);
    } else {
        errno = EINVAL;
        return NULL;
    }

    if (!f) {
        return NULL;
    }

    pid_t pid = fork();
    if (pid == 0) {
        if (mode[0] == 'r') {
            dup2(fds[1], STDOUT_FILENO);
            close(fds[1]);
        } else {
            dup2(fds[0], STDIN_FILENO);
            close(fds[0]);
        }

        char *const args[] = { "sh", "-c", (char *) command, NULL };

        execve("/bin/sh", args, environ);
        _exit(127);
    } else if (pid == -1) {
        fclose(f);
        return NULL;
    }

    __p_child_pid = pid;
    if (mode[0] == 'r') {
        close(fds[1]);
    } else {
        close(fds[0]);
    }

    return f;
}
