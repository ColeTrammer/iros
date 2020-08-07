#define __libc_internal

#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>

int pclose(FILE *stream) {
    if (__p_child_pid == 0) {
        errno = EINVAL;
        return -1;
    }

    int wstatus;
    while (!waitpid(__p_child_pid, &wstatus, 0))
        ;
    __p_child_pid = 0;

    if (fclose(stream) || wstatus == -1) {
        return -1;
    }

    return WEXITSTATUS(wstatus);
}
