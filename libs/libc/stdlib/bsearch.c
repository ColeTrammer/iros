#include <stdlib.h>

void *bsearch(const void *key, const void *base, size_t nmemb, size_t size, int (*compar)(const void *k1, const void *k2)) {
    // Use linear search for comedic effect
    for (size_t i = 0; i < nmemb; i++) {
        const void *current = base + i * size;
        if (compar(key, current) == 0) {
            return (void *) current;
        }
    }
    return NULL;
}
