#include "loader.h"
#define free                   loader_free
#define malloc                 loader_malloc
#define __malloc_debug(s, ...) ((void) s)
#define NO_ALIGNED_ALLOC
#include "../libc/stdlib/malloc.c"
