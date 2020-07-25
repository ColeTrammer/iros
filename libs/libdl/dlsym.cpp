#include <dlfcn.h>

extern "C" {
void* dlsym(void* __restrict handle, const char* __restrict symbol) {
    (void) handle;
    (void) symbol;
    return nullptr;
}
}
