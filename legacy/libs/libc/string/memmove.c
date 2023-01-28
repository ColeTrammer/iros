#include <string.h>

#if (defined(__is_kernel) || defined(__is_libk)) && defined(KERNEL_MEMCPY_DEBUG)
#include <stdint.h>
#include <kernel/hal/output.h>
#undef memmove
void *memmove(void *dest, const void *src, size_t n, int line, const char *func) {
    debug_log("memmove call: [ %d, %s, %#.16lX, %#.16lX, %lu ]\n", line, func, (uintptr_t) dest, (uintptr_t) src, n);
#else
void *memmove(void *dest, const void *src, size_t n) {
#endif /* #if (defined(__is_kernel) || defined(__is_libk)) && defined(KERNEL_MEMCPY_DEBUG) */
    unsigned char *buffer = (unsigned char *) dest;
    const unsigned char *source = (const unsigned char *) src;
    if (source < buffer) {
        for (size_t i = n; i > 0; i--) {
            buffer[i - 1] = source[i - 1];
        }
    } else {
        for (size_t i = 0; i < n; i++) {
            buffer[i] = source[i];
        }
    }
    return buffer;
}
