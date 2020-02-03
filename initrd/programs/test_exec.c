#include <assert.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>

int main(/* int argc, char **argv, char **envp */) {
    int fd = syscall(SC_OPENAT, AT_FDCWD, "/usr/include/sys/syscall.h", O_RDONLY, 0);
    assert(fd != -1);

    char buf[4096];
    assert(syscall(SC_READ, fd, buf, 4096) >= 0);

    puts(buf);

    assert(syscall(SC_CLOSE, fd) == 0);
    return 0;
}