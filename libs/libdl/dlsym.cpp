#define __libc_internal
#include <dlfcn.h>

#include "handle.h"

extern "C" {
void* dlsym(void* __restrict _handle, const char* __restrict symbol) {
    Handle* handle = reinterpret_cast<Handle*>(_handle);
    auto* fetched_symbol = handle->dynamic_object.lookup_symbol(symbol);
    if (!fetched_symbol) {
        __dl_set_error("could not find symbol `%s'", symbol);
        return nullptr;
    }

    return reinterpret_cast<void*>(handle->executable->base() + fetched_symbol->st_value);
}
}
