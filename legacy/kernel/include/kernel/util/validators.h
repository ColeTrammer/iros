#ifndef _KERNEL_UTIL_VALIDATORS_H
#define _KERNEL_UTIL_VALIDATORS_H 1

#include <stddef.h>

#define __DO_VALIDATE(a1, a2, f, r) \
    do {                            \
        int ret = f(a1, a2);        \
        if (ret < 0) {              \
            r(ret);                 \
        }                           \
    } while (0)

#define __RETURN(r)         return r
#define VALIDATE(a1, a2, f) __DO_VALIDATE(a1, a2, f, __RETURN)

int validate_string(const char *s, int unused);
int validate_string_array(char **arr, int unused);
int validate_path(const char *s, int unused);
int validate_path_or_null(const char *s, int dont_tolerate_null_after_all);
int validate_write(void *buffer, size_t size);
int validate_read(const void *buffer, size_t size);
int validate_write_or_null(void *buffer, size_t size);
int validate_read_or_null(const void *buffer, size_t size);
int validate_signal_number(int signum, int unused);
int validate_positive(int n, int accept_zero);

// Sanity check when accessing kernel memory
int validate_kernel_read(const void *buffer, size_t size);

#endif /* _KERNEL_UTIL_VALIDATORS_H */
