#include <dlfcn.h>

extern "C" {
char* dlerror() {
    return nullptr;
}
}
