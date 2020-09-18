#include <assert.h>
#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>

static void *handle;
static int (*test)(void);

int executable_symbol() {
    return 67;
}

static void *thread_start(void *arg __attribute__((unused))) {
    fprintf(stderr, "THREAD 2: %d\n", test());
    fprintf(stderr, "THREAD 2: %d\n", test());
    fprintf(stderr, "THREAD 2: %d\n", test());
    return NULL;
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

    handle = dlopen("/usr/lib/libsharedtest2.so", RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        fprintf(stderr, "%s\n", dlerror());
        return 1;
    }

    void *handle2 = dlopen("/usr/lib/libsharedtest2.so", RTLD_NOW | RTLD_GLOBAL);
    assert(handle == handle2);
    dlclose(handle2);

    test = dlsym(handle, "test");
    if (!test) {
        fprintf(stderr, "%s\n", dlerror());
        return 1;
    }

    printf("%d\n", test());
    printf("%d\n", test());
    printf("%d\n", test());

    Dl_info info;
    assert(dladdr(test, &info) != 0);
    printf("OBJ PATH: %s\n", info.dli_fname);
    printf("FUN NAME: %s\n", info.dli_sname);
    assert(info.dli_saddr == test);

    assert(dladdr(test - 1, &info) != 0);
    printf("OBJ PATH: %s\n", info.dli_fname);
    assert(!info.dli_sname);

    assert(dladdr(test + 1, &info) != 0);
    assert(info.dli_fname);
    assert(info.dli_sname);
    assert(info.dli_saddr == test);

    void *global_handle = dlopen(NULL, RTLD_LAZY);
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

    pthread_t new_thread;
    assert(!pthread_create(&new_thread, NULL, thread_start, NULL));
    assert(!pthread_join(new_thread, NULL));

    if (dlclose(handle)) {
        fprintf(stderr, "%s\n", dlerror());
        return 1;
    }
    return 0;
}
