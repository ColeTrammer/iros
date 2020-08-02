#ifndef _TLS_RECORD_H
#define _TLS_RECORD_H 1

#include <bits/tls_record.h>

#include "loader.h"

#define TLS_RECORD_PROGRAM 1

struct tls_record *add_tls_record(void *tls_image, size_t tls_image_size, size_t tls_align, int flags) LOADER_PRIVATE;
void remove_tls_record(struct tls_record *record) LOADER_PRIVATE;
struct tls_record *tls_record_for(size_t m) LOADER_PRIVATE;
size_t tls_num_records(void) LOADER_PRIVATE;
size_t tls_initial_image_size(void) LOADER_PRIVATE;
size_t tls_initial_image_alignment(void) LOADER_PRIVATE;

#endif /* _TLS_RECORD_H */
