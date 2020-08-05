#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>

int main() {
    // Test error conditions
    assert(!dlopen("/usr/lib/libsharedtest2.so", RTLD_NOW | RTLD_LAZY));
    fprintf(stderr, "%s\n", dlerror());
    assert(!dlopen("/usr/lib/libsharedtest2.so", 0));
    fprintf(stderr, "%s\n", dlerror());
    assert(!dlopen("/usr/lib/libsharedtest2.so", ~(RTLD_LAZY)));
    fprintf(stderr, "%s\n", dlerror());
    assert(!dlopen("/xyz", RTLD_LAZY));
    fprintf(stderr, "%s\n", dlerror());

    void *handle = dlopen("/usr/lib/libsharedtest2.so", RTLD_NOW | RTLD_LOCAL);
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
    printf("%d\n", test());
    printf("%d\n", test());

    if (dlclose(handle)) {
        fprintf(stderr, "%s\n", dlerror());
        return 1;
    }
    return 0;
}
