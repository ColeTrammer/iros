#include <assert.h>
#include <bits/lock.h>
#include <bits/malloc.h>

void *operator new(size_t size) {
    return malloc(size);
}

void *operator new[](size_t size) {
    return malloc(size);
}

void operator delete(void *p) {
    free(p);
}

void operator delete(void *p, size_t) {
    free(p);
}

void operator delete[](void *p) {
    free(p);
}

void operator delete[](void *p, size_t) {
    free(p);
}

extern "C" void __cxa_pure_virtual() {
    assert(false);
}

extern "C" void __cxa_finalize() {}

namespace __cxxabiv1 {
/* guard variables */

/* The ABI requires a 64-bit type.  */
__extension__ typedef int __guard __attribute__((mode(__DI__)));

extern "C" int __cxa_guard_acquire(__guard *g) {
    return __trylock((unsigned int *) g);
}

extern "C" void __cxa_guard_release(__guard *) {}

extern "C" void __cxa_guard_abort(__guard *) {}

}
