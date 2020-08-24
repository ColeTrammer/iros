#ifndef _KERNEL_UTIL_RING_BUFFER_H
#define _KERNEL_UTIL_RING_BUFFER_H 1

#include <kernel/util/macros.h>

struct ring_buffer {
    void *buffer;
    size_t max;
    size_t head;
    size_t tail;
    bool full;
};

void init_ring_buffer(struct ring_buffer *rb, size_t max);
void kill_ring_buffer(struct ring_buffer *rb);

void ring_buffer_advance(struct ring_buffer *rb, size_t amount);
void ring_buffer_copy(struct ring_buffer *rb, size_t offset, void *data, size_t amount);
void ring_buffer_user_read(struct ring_buffer *rb, void *data, size_t amount);
void ring_buffer_write(struct ring_buffer *rb, const void *buffer, size_t amount);
void ring_buffer_user_write(struct ring_buffer *rb, const void *data, size_t amount);

static inline size_t ring_buffer_size(struct ring_buffer *rb) {
    if (rb->full) {
        return rb->max;
    }
    if (rb->tail < rb->head) {
        return rb->tail + rb->max - rb->head;
    }
    return rb->tail - rb->head;
}

static inline size_t ring_buffer_space(struct ring_buffer *rb) {
    return rb->max - ring_buffer_size(rb);
}

static inline bool ring_buffer_empty(struct ring_buffer *rb) {
    return rb->head == rb->tail && !rb->full;
}

static inline bool ring_buffer_max(struct ring_buffer *rb) {
    return rb->max;
}

static inline bool ring_buffer_full(struct ring_buffer *rb) {
    return rb->full;
}

static inline bool ring_buffer_dead(struct ring_buffer *rb) {
    return !rb->buffer;
}

#endif /* _KERNEL_UTIL_RING_BUFFER_H */
