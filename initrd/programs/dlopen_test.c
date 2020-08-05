#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>

int executable_symbol() {
    return 67;
}

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

    void *handle = dlopen("/usr/lib/libsharedtest2.so", RTLD_NOW | RTLD_GLOBAL);
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

    void *global_handle = dlopen(NULL, 0);
    assert(global_handle);

    int (*sym)(void) = dlsym(global_handle, "executable_symbol");
    if (!sym) {
        fprintf(stderr, "%s\n", dlerror());
        return 1;
    }

    printf("%d\n", sym());
    printf("%d\n", sym());
    printf("%d\n", sym());

    test = dlsym(global_handle, "test");
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
