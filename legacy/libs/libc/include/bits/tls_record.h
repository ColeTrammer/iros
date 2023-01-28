#ifndef _BITS_TLS_RECORD_H
#define _BITS_TLS_RECORD_H 1

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct tls_record {
    void *tls_image;
    size_t tls_size;
    union {
        size_t tls_offset; /* For records describing the initial tls image */
        size_t tls_align;  /* For records describing dynamically loaded modules */
    };
    size_t tls_generation_number;
};

extern __attribute__((weak)) size_t __loader_tls_generation_number;
extern __attribute__((weak)) struct tls_record *__loader_tls_records;

__attribute__((weak)) size_t __loader_tls_num_records(void);
__attribute__((weak)) size_t __loader_tls_initial_record_count(void);
__attribute__((weak)) size_t __loader_tls_initial_image_size(void);
__attribute__((weak)) size_t __loader_tls_initial_image_alignment(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITS_TLS_RECORD_H */
