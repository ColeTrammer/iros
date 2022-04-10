#ifdef __is_shared

#define __libc_internal

#include <assert.h>

void ___tls_get_addr() {
    assert(0);
}

#endif /* __is_shared */
