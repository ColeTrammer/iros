#include <string.h>

#if (defined(__is_kernel) || defined(__is_libk)) && defined(KERNEL_MEMCPY_DEBUG)
#include <stdint.h>
#include <kernel/hal/output.h>
#undef memcpy
void *memcpy(void *__restrict dest, const void *__restrict src, size_t n, int line, const char *func) {
#define memcpy(d, s, n) memcpy(dest, src, n, __LINE__, __func__) {
    debug_log("memcpy call: [ %d, %s, %#.16lX, %#.16lX, %lu ]\n", line, func, (uintptr_t) dest, (uintptr_t) src, n);
#else
void *memcpy(void *__restrict dest, const void *__restrict src, size_t n) {
#endif /* (defined(__is_kernel) || defined(__is_libk)) && defined(KERNEL_MEMCPY_DEBUG) */
    unsigned char *buffer = (unsigned char*) dest;
    const unsigned char *source = (const unsigned char*) src;
    for (size_t i = 0; i < n; i++) {
        buffer[i] = source[i];
    }
    return buffer;
}