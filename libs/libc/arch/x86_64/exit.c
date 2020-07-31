#define __libc_internal

#include <bits/dynamic_elf_object.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __is_static
extern void (*__fini_array_start[])(void);
extern void (*__fini_array_end[])(void);

extern void _fini(void);
#endif /* __is_static */

__attribute__((__noreturn__)) void exit(int status) {
    __on_exit();

#ifdef __is_static
    size_t count = (__fini_array_end - __fini_array_start);
    for (size_t i = count; i > 0; i--) {
        __fini_array_start[i - 1]();
    }

    _fini();
#else
    for (struct dynamic_elf_object *obj = __loader_get_dynamic_object_head(); obj; obj = obj->next) {
        __loader_call_fini_functions(obj);
    }
#endif /* __is_static */

    fflush_unlocked(NULL);

    _exit(status);

    __builtin_unreachable();
}
