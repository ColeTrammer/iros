#include <assert.h>
#include <bits/malloc.h>

void* operator new(size_t size) {
    return malloc(size);
}

void* operator new[](size_t size) {
    return malloc(size);
}

void operator delete(void* p) {
    free(p);
}

void operator delete(void* p, size_t) {
    free(p);
}

void operator delete[](void* p) {
    free(p);
}

void operator delete[](void* p, size_t) {
    free(p);
}

extern "C" void __cxa_pure_virtual() {
    assert(false);
}