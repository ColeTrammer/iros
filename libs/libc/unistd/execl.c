#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

int execl(const char *path, const char *arg, ...) {
    assert(arg);

    va_list args;
    va_start(args, arg);

    size_t i = 1;
    size_t max = 50;
    char **arg_list = malloc(max * sizeof(char *));

    char *a;
    while ((a = va_arg(args, char *))) {
        if (i + 1 >= max) {
            max *= 2;
            arg_list = realloc(arg_list, max * sizeof(char *));
        }

        arg_list[i++] = a;
    }

    arg_list[0] = (char *) arg;
    arg_list[i] = NULL;

    int ret = execve(path, arg_list, environ);

    free(arg_list);
    va_end(args);
    return ret;
}
