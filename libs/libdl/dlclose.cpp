#include <dlfcn.h>

#include "handle.h"

extern "C" {
int dlclose(void* handle) {
    delete reinterpret_cast<Handle*>(handle);
    return 0;
}
}
