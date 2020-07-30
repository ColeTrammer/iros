#include "loader.h"
#define free                   loader_free
#define malloc                 loader_malloc
#define calloc                 loader_calloc
#define realloc                loader_realloc
#define __malloc_debug(s, ...) ((void) s)
#include "../libc/stdlib/malloc.c"
#undef free
#undef malloc
#undef calloc
#undef realloc

void *malloc(size_t size) {
    return loader_malloc(size);
}

void *calloc(size_t n, size_t m) {
    return loader_calloc(n, m);
}

void *realloc(void *p, size_t n) {
    return loader_realloc(p, n);
}

void free(void *p) {
    loader_free(p);
}
