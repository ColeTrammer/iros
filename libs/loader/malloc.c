#include "loader.h"
#define free                   loader_free
#define malloc                 loader_malloc
#define calloc                 loader_calloc
#define realloc                loader_realloc
#define __malloc_debug(s, ...) ((void) s)
#define NO_ALIGNED_ALLOC
#include "../libc/stdlib/malloc.c"
