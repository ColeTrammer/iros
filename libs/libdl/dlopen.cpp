#include <dlfcn.h>

extern "C" {
void* dlopen(const char* file, int flags) {
    (void) file;
    (void) flags;
    return nullptr;
}
}
