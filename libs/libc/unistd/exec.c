#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define EXEC_BUF_INC 100

int execl(const char *path, const char *arg, ...) {
    assert(arg);

    va_list args;
    va_start(args, arg);

    char **arg_list = malloc(EXEC_BUF_INC * sizeof(char *));
    size_t i = 1;
    size_t max = 0;

    char *a;
    while ((a = va_arg(args, char *))) {
        if (i + 1 >= max) {
            max += EXEC_BUF_INC;
            arg_list = realloc(arg_list, max * sizeof(char *));
        }

        arg_list[i++] = a;
    }

    arg_list[0] = (char *) arg;
    arg_list[i] = NULL;

    int ret = execve(path, arg_list, environ);

    va_end(args);
    return ret;
}

int execle(const char *path, const char *arg, ...) {
    assert(arg);

    va_list args;
    va_start(args, arg);

    char **arg_list = malloc(EXEC_BUF_INC * sizeof(char *));
    size_t i = 1;
    size_t max = 0;

    char *a;
    while ((a = va_arg(args, char *))) {
        if (i + 1 >= max) {
            max += EXEC_BUF_INC;
            arg_list = realloc(arg_list, max * sizeof(char *));
        }

        arg_list[i++] = a;
    }

    arg_list[0] = (char *) arg;
    arg_list[i] = NULL;

    int ret = execve(path, arg_list, va_arg(args, char **));

    va_end(args);
    return ret;
}

int execlp(const char *name, const char *arg, ...) {
    assert(arg);

    va_list args;
    va_start(args, arg);

    char **arg_list = malloc(EXEC_BUF_INC * sizeof(char *));
    size_t i = 1;
    size_t max = 0;

    char *a;
    while ((a = va_arg(args, char *))) {
        if (i + 1 >= max) {
            max += EXEC_BUF_INC;
            arg_list = realloc(arg_list, max * sizeof(char *));
        }

        arg_list[i++] = a;
    }

    arg_list[0] = (char *) arg;
    arg_list[i] = NULL;

    int ret = execvp(name, arg_list);

    va_end(args);
    return ret;
}

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

    bool e_access = false;
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

        test_dir = strtok(NULL, ":");
    }

    free(path);
    errno = e_access ? EACCES : error;
    return -1;
}