#include <dlfcn.h>
#include <stdio.h>

int main() {
    void *handle = dlopen("/usr/lib/libsharedtest.so", RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        fprintf(stderr, "%s\n", dlerror());
        return 1;
    }

    int (*test)(void) = dlsym(handle, "test");
    if (!test) {
        fprintf(stderr, "%s\n", dlerror());
        return 1;
    }

    printf("%d\n", test());

    if (dlclose(handle)) {
        fprintf(stderr, "%s\n", dlerror());
        return 1;
    }
    return 0;
}
