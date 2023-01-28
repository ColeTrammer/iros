#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int execvpe(const char *file, char *const argv[], char *const envp[]) {
    // Don't search path
    if (strchr(file, '/') != NULL) {
        return execve(file, argv, envp);
    }

    char *pathvar = getenv("PATH");
    if (!pathvar) {
        pathvar = "/bin:/usr/bin";
    }

    bool e_access = false;
    int error = 0;
    char *path = strdup(pathvar);

    char *save = NULL;
    char *test_dir = strtok_r(path, ":", &save);
    while (test_dir) {
        char *test_file = malloc(strlen(test_dir) + strlen(file) + 2);
        strcpy(test_file, test_dir);
        strcat(test_file, "/");
        strcat(test_file, file);

        int ret = execve(test_file, argv, envp);
        assert(ret == -1);
        if (errno == EACCES) {
            e_access = true;
        } else if (errno == ENOEXEC) {
            size_t num_args = 0;
            while (argv[num_args++] != NULL)
                ;

            char **new_args = malloc((num_args + 1) + sizeof(char *));
            new_args[0] = "/bin/sh";
            new_args[1] = test_file;
            memcpy(&new_args[2], &argv[1], (num_args - 1) * sizeof(char *));
            execve(new_args[0], new_args, envp);
            free(new_args);
            return -1;
        } else {
            error = errno;
        }

        free(test_file);

        test_dir = strtok_r(NULL, ":", &save);
    }

    free(path);
    errno = e_access ? EACCES : error;
    return -1;
}
