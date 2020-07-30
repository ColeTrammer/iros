#define __libc_internal
#include <dlfcn.h>
#include <stdio.h>

void* dlsym(void* __restrict _handle, const char* __restrict symbol) {
    (void) _handle;
    __dl_set_error("could not find symbol `%s'", symbol);
    return NULL;
}
