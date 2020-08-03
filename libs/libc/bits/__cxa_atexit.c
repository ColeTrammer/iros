#include <bits/cxx.h>
#include <stdlib.h>
#include <sys/param.h>

size_t __exit_functions_max;
size_t __exit_functions_size;
struct exit_function_entry *__exit_functions;

int __cxa_atexit(void (*f)(void *closure), void *closure, void *dso) {
    if (__exit_functions_size >= __exit_functions_max) {
        __exit_functions_max = MAX(20, __exit_functions_max * 2);
        struct exit_function_entry *new_exit_functions =
            realloc(__exit_functions, __exit_functions_max * sizeof(struct exit_function_entry));
        if (!new_exit_functions) {
            return 1;
        }
        __exit_functions = new_exit_functions;
    }

    struct exit_function_entry *entry = &__exit_functions[__exit_functions_size++];
    entry->f = f;
    entry->closure = closure;
    entry->dso = dso;
    return 0;
}
