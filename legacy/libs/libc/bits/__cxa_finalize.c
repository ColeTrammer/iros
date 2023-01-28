#include <bits/cxx.h>
#include <string.h>

void __cxa_finalize(void *dso) {
restart:
    for (size_t i = __exit_functions_size; i > 0; i--) {
        struct exit_function_entry *entry = &__exit_functions[i - 1];
        void (*f)(void *closure) = entry->f;
        void *closure = entry->closure;

        if (!dso || entry->dso == dso) {
            // Delete the entry from the array.
            memmove(entry, entry + 1, sizeof(struct exit_function_entry) * (__exit_functions_size-- - i));

            size_t expected_size = __exit_functions_size;
            f(closure);

            // Exit function called atexit
            if (__exit_functions_size != expected_size) {
                goto restart;
            }
        }
    }
}
