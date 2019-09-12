#include <unistd.h>

int execvp(const char *file, char *const argv[]) {
    return execve(file, argv, environ);
}