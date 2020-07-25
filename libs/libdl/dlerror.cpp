#define __libc_internal
#include <dlfcn.h>

extern "C" {
char* dlerror() {
    if (__dl_has_error) {
        __dl_has_error = false;
        return __dl_error;
    }
    return nullptr;
}
}
