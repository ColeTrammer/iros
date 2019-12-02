#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

int main() {
    {
        uint32_t *a = aligned_alloc(0x1000, 16);
        *a = 0x10;
        assert(((uintptr_t) a) % 0x1000 == 0);
        free(a);
    }
    {
        uint32_t *a = aligned_alloc(0x1000, 16);
        *a = 0x10;
        assert(((uintptr_t) a) % 0x1000 == 0);
        free(a);

        free(malloc(0x500));
    }
    {
        uint32_t *a = aligned_alloc(0x1000, 16);
        *a = 0x10;
        assert(((uintptr_t) a) % 0x1000 == 0);
        free(a);
    }
    return 0;
}