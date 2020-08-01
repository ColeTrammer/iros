#include "loader.h"
#define free                   loader_free
#define malloc                 loader_malloc
#define aligned_alloc          loader_aligned_alloc
#define calloc                 loader_calloc
#define realloc                loader_realloc
#define __malloc_debug(s, ...) ((void) s)
#include "../libc/stdlib/malloc.c"
#undef free
#undef malloc
#undef aligned_alloc
#undef calloc
#undef realloc

LOADER_HIDDEN_EXPORT(loader_malloc, __loader_malloc);
LOADER_HIDDEN_EXPORT(loader_free, __loader_free);
LOADER_HIDDEN_EXPORT(loader_calloc, __loader_calloc);
LOADER_HIDDEN_EXPORT(loader_realloc, __loader_realloc);
LOADER_HIDDEN_EXPORT(loader_aligned_alloc, __loader_aligned_alloc);
