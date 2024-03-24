#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define QSORT_AT(p, in, size) ((void *) (((uintptr_t)(p)) + (in) * (size)))

// Use selection sort for simplicity
void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *a, const void *b)) {
    if (base == NULL || nmemb == 0 || size == 0 || compar == NULL) {
        return;
    }

    void *temp;
    if (size > 0x200) {
        temp = malloc(size);
    } else {
        temp = alloca(size);
    }

    for (size_t i = 0; i < nmemb - 1; i++) {
        void *to_replace = QSORT_AT(base, i, size);
        void *min = to_replace;

        for (size_t j = i + 1; j < nmemb; j++) {
            void *curr = QSORT_AT(base, j, size);
            if (compar(curr, min) < 0) {
                min = curr;
            }
        }

        if (to_replace == min) {
            continue;
        }

        // Do swap
        memcpy(temp, to_replace, size);
        memcpy(to_replace, min, size);
        memcpy(min, temp, size);
    }

    if (size > 0x200) {
        free(temp);
    }
}
