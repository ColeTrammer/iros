#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main() {
    char path[2 * PATH_MAX] = { 0 };
    memset(path, '/', PATH_MAX + 1);
    if (open(path, O_RDONLY)) {
        perror("open");
    }

    if (open((char*) 0x12345678ULL, O_RDONLY)) {
        perror("open");
    }

    char* a[2] = { (char*) 0x12345678ULL, NULL };
    if (execve("/bin/echo", a, environ)) {
        perror("execve");
    }

    if (read(FOPEN_MAX, path, 0)) {
        perror("read");
    }

    if (kill(0, _NSIG + 1000)) {
        perror("kill");
    }

    if (write(STDOUT_FILENO, (char*) 0x12345678, 0x1000)) {
        perror("write");
    }

    return 0;
}
