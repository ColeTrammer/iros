#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int execv(const char *path, char *const argv[]) {
    return execve(path, argv, environ);
}

int execvp(const char *file, char *const argv[]) {
    return execvpe(file, argv, environ);
}

int execvpe(const char *file, char *const argv[], char *const envp[]) {
    // Don't search path
    if (strchr(file, '/') != NULL) {
        return execve(file, argv, envp);
    }

    char *pathvar = getenv("PATH");
    if (!pathvar) {
        pathvar = "/bin:/usr/bin";
    }

    int error = 0;
    char *path = strdup(pathvar);

    char *test_dir = strtok(path, ":");
    while (test_dir) {
        char *test_file = malloc(strlen(test_dir) + strlen(file) + 2);
        strcpy(test_file, test_dir);
        strcat(test_file, "/");
        strcat(test_file, file);

        int ret = execve(test_file, argv, envp);
        assert(ret == -1);
        if (errno == ENOENT) {
            if (error == 0) { 
                error = ENOENT;
            }
        } else {
            error = errno;
        }

        free(test_file);

        test_dir = strtok(NULL, ":");
    }

    free(path);
    errno = error;
    return -1;
}