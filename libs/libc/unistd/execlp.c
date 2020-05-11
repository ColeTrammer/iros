#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

int execlp(const char *name, const char *arg, ...) {
    assert(arg);

    va_list args;
    va_start(args, arg);

    char **arg_list = malloc(50 * sizeof(char *));
    size_t i = 1;
    size_t max = 0;

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

    int ret = execvp(name, arg_list);

    va_end(args);
    free(arg_list);
    return ret;
}
