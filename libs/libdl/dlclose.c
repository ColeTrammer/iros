#define __libc_internal

#include <bits/dynamic_elf_object.h>
#include <dlfcn.h>

int dlclose(void* handle) {
    if (!handle) {
        return 0;
    }

    struct dynamic_elf_object* obj = handle;
    if (obj->is_program) {
        return 0;
    }

    __loader_call_fini_functions(obj);
    __loader_free_dynamic_elf_object(obj);
    return 0;
}
