#ifndef _BITS_TLS_RECORD_H
#define _BITS_TLS_RECORD_H 1

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct tls_record {
    void *tls_image;
    size_t tls_size;
    size_t tls_offset;
    size_t tls_module_id;
};

__attribute__((weak)) struct tls_record *__loader_tls_record_for(size_t m);
__attribute__((weak)) size_t __loader_tls_num_records(void);
__attribute__((weak)) size_t __loader_tls_initial_image_size(void);
__attribute__((weak)) size_t __loader_tls_initial_image_alignment(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITS_TLS_RECORD_H */
