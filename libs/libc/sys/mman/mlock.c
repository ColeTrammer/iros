#include <sys/mman.h>

int mlock(void *addr, size_t length) {
    (void) addr;
    (void) length;
    return 0;
}
