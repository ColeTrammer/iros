#include <dlfcn.h>

extern "C" {
int dlclose(void* handle) {
    (void) handle;
    return 0;
}
}
