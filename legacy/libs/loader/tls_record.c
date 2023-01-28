#include <stdatomic.h>
#include <sys/param.h>

#include "tls_record.h"

__attribute__((nocommon)) size_t tls_generation_number;
LOADER_HIDDEN_EXPORT(tls_generation_number, __loader_tls_generation_number);
__attribute__((nocommon)) struct tls_record *tls_records;
LOADER_HIDDEN_EXPORT(tls_records, __loader_tls_records);

static size_t tls_initial_image_sz;
static size_t tls_initial_image_align;
static size_t s_tls_initial_record_count;
static size_t tls_record_count;
static size_t tls_record_max;

size_t add_tls_record(void *tls_image, size_t tls_image_size, size_t tls_align, int flags) {
    struct tls_record *record = NULL;
    if (tls_record_max >= tls_record_count) {
        tls_record_max = MAX(20, tls_record_max * 2);
        tls_records = loader_realloc(tls_records, tls_record_max * sizeof(struct tls_record));
    }

    // The first tls record is reserved for the program executable even if it has no tls segment.
    if (tls_record_count == 0 && !(flags & TLS_RECORD_PROGRAM)) {
        tls_records[0].tls_image = NULL;
        tls_records[0].tls_generation_number = 0;
        tls_record_count++;
        s_tls_initial_record_count++;
    }

    size_t tls_module_id = 0;
    for (size_t i = s_tls_initial_record_count; i < tls_record_count; i++) {
        if (tls_records[i].tls_image == NULL) {
            tls_module_id = i;
            goto update_record;
        }
    }

    tls_module_id = tls_record_count++;

update_record:
    record = &tls_records[tls_module_id++]; /* increment tls_module_id because their indexes start at 1 */
    record->tls_image = tls_image;

    if (flags & TLS_INITIAL_IMAGE) {
        record->tls_size = ALIGN_UP(tls_image_size, tls_align);
        record->tls_offset = ALIGN_UP(tls_initial_image_sz + tls_image_size, tls_align);

        tls_initial_image_align = MAX(tls_initial_image_align, tls_align);
        tls_initial_image_sz = record->tls_offset;

        s_tls_initial_record_count++;
        record->tls_generation_number = 0;
    } else {
        record->tls_size = tls_image_size;
        record->tls_align = tls_align;

        // FIXME: does this need a stronger memory ordering?
        record->tls_generation_number = 1 + atomic_fetch_add_explicit(&tls_generation_number, 1, memory_order_relaxed);
    }

#ifdef LOADER_TLS_DEBUG
    loader_log("tls record input: { %lu, %lu }", tls_image_size, tls_align);
    loader_log("tls record added: { %lu, %lu, %lu, %lu, %lu, %p }", record->tls_offset, record->tls_size, tls_module_id,
               s_tls_initial_record_count, tls_record_count, record->tls_image);
#endif /* LOADER_TLS_DEBUG */

    return tls_module_id;
}

void remove_tls_record(size_t module_id) {
    tls_records[module_id - 1].tls_image = NULL;
    tls_records[module_id - 1].tls_generation_number = atomic_load_explicit(&tls_generation_number, memory_order_relaxed);

    // FIXME: does this need a stronger memory ordering?
    atomic_fetch_add_explicit(&tls_generation_number, 1, memory_order_relaxed);
}

size_t tls_num_records(void) {
    return tls_record_count;
}
LOADER_HIDDEN_EXPORT(tls_num_records, __loader_tls_num_records);

size_t tls_initial_record_count(void) {
    return s_tls_initial_record_count;
}
LOADER_HIDDEN_EXPORT(tls_initial_record_count, __loader_tls_initial_record_count);

size_t tls_initial_image_size(void) {
    return tls_initial_image_sz;
}
LOADER_HIDDEN_EXPORT(tls_initial_image_size, __loader_tls_initial_image_size);

size_t tls_initial_image_alignment(void) {
    return tls_initial_image_align;
}
LOADER_HIDDEN_EXPORT(tls_initial_image_alignment, __loader_tls_initial_image_alignment);
