#ifndef _TLS_RECORD_H
#define _TLS_RECORD_H 1

#include <bits/tls_record.h>

#include "loader.h"

#define TLS_RECORD_PROGRAM 1
#define TLS_INITIAL_IMAGE  2

extern LOADER_PRIVATE size_t tls_generation_number;
extern LOADER_PRIVATE struct tls_record *tls_records;

size_t add_tls_record(void *tls_image, size_t tls_image_size, size_t tls_align, int flags) LOADER_PRIVATE;
void remove_tls_record(size_t tls_module_id) LOADER_PRIVATE;
size_t tls_num_records(void) LOADER_PRIVATE;
size_t tls_initial_record_count(void) LOADER_PRIVATE;
size_t tls_initial_image_size(void) LOADER_PRIVATE;
size_t tls_initial_image_alignment(void) LOADER_PRIVATE;

#endif /* _TLS_RECORD_H */
