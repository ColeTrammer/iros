#include <dlfcn.h>

int dlclose(void* handle) {
    (void) handle;
    return 0;
}
