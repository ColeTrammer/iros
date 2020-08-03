#define __libc_internal

#include <bits/dynamic_elf_object.h>
#include <dlfcn.h>

int dlclose(void* handle) {
    if (!handle) {
        return 0;
    }

    __loader_call_fini_functions(handle);
    __loader_remove_dynamic_object(handle);
    __loader_free_dynamic_elf_object(handle);
    return 0;
}
