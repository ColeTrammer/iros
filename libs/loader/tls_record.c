#include <sys/param.h>

#include "tls_record.h"

static size_t tls_initial_image_sz;
static size_t tls_initial_image_align;
static size_t tls_record_count;
static size_t tls_record_max;
static struct tls_record **tls_records;

struct tls_record *add_tls_record(void *tls_image, size_t tls_image_size, size_t tls_align, int flags) {
    struct tls_record *record = loader_malloc(sizeof(struct tls_record));
    record->tls_image = tls_image;
    record->tls_size = ALIGN_UP(tls_image_size, tls_align);
    record->tls_offset = ALIGN_UP(tls_initial_image_sz + tls_image_size, tls_align);

    tls_initial_image_align = MAX(tls_initial_image_align, tls_align);
    tls_initial_image_sz = record->tls_offset;

    if (tls_record_max >= tls_record_count) {
        tls_record_max = MAX(20, tls_record_max * 2);
        tls_records = loader_realloc(tls_records, tls_record_max * sizeof(struct tls_record));
    }

    // The first tls record is reserved for the program executable even if it has no tls segment.
    if (tls_record_count == 0 && !(flags & TLS_RECORD_PROGRAM)) {
        tls_record_count++;
    }
    tls_records[tls_record_count++] = record;
    record->tls_module_id = tls_record_count;

#ifdef LOADER_TLS_DEBUG
    loader_log("tls record input: { %lu, %lu }", tls_image_size, tls_align);
    loader_log("tls record added: { %lu, %lu, %lu, %p }", record->tls_offset, record->tls_size, tls_record_count, record->tls_image);
#endif /* LOADER_TLS_DEBUG */

    return record;
}

void remove_tls_record(struct tls_record *record) {
    // FIXME: implement for dlclose()
    (void) record;
}

struct tls_record *tls_record_for(size_t m) {
    return tls_records[m - 1];
}
LOADER_HIDDEN_EXPORT(tls_record_for, __loader_tls_record_for);

size_t tls_num_records(void) {
    return tls_record_count;
}
LOADER_HIDDEN_EXPORT(tls_num_records, __loader_tls_num_records);

size_t tls_initial_image_size(void) {
    return tls_initial_image_sz;
}
LOADER_HIDDEN_EXPORT(tls_initial_image_size, __loader_tls_initial_image_size);

size_t tls_initial_image_alignment(void) {
    return tls_initial_image_align;
}
LOADER_HIDDEN_EXPORT(tls_initial_image_alignment, __loader_tls_initial_image_alignment);
