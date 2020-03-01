#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

int main() {
    uint8_t *text = (uint8_t *) (uintptr_t) &main;
    *text = 0;
    assert(false);
    return 1;
}