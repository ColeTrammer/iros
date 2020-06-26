#include <stdlib.h>
#include <kernel/hal/hal.h>
#include <kernel/util/random.h>

uint32_t get_random_bytes() {
#if ARCH == X86_64
    if (cpu_supports_rdrand()) {
        uint32_t data;
        asm volatile("rdrand_loop:\n"
                     "rdrand %0\n"
                     "jnc rdrand_loop\n"
                     : "=r"(data)
                     :
                     :);
        return data;
    }
#endif /* ARCH == X86_64 */

    // FIXME: use a better algorithm
    uint32_t r1 = (rand() & 0xFFFF);
    uint32_t r2 = (rand() & 0xFFFF) << 16;
    return r1 | r2;
}
