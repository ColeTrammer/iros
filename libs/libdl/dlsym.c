#define __libc_internal
#include <bits/dynamic_elf_object.h>
#include <dlfcn.h>
#include <elf.h>
#include <stdio.h>

void* dlsym(void* __restrict _handle, const char* __restrict symbol) {
    struct dynamic_elf_object* handle = _handle;
    const Elf64_Sym* sym = __loader_lookup_symbol(handle, symbol);
    if (!sym) {
        __dl_set_error("could not find symbol `%s'", symbol);
        return NULL;
    }
    return (void*) (sym->st_value + handle->relocation_offset);
}
