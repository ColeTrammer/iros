#include <stdlib.h>
#include <string.h>

#include <kernel/util/ring_buffer.h>

void init_ring_buffer(struct ring_buffer *rb, size_t max) {
    rb->buffer = malloc(max);
    rb->max = max;
    rb->head = rb->tail = 0;
    rb->full = false;
}

void kill_ring_buffer(struct ring_buffer *rb) {
    free(rb->buffer);
    rb->buffer = NULL;
}

void ring_buffer_advance(struct ring_buffer *rb, size_t amount) {
    rb->head += amount;
    rb->head %= rb->max;
    rb->full = false;
}

void ring_buffer_copy(struct ring_buffer *rb, size_t offset, void *buffer, size_t amount) {
    size_t head = (rb->head + offset) % rb->max;
    if (head + amount > rb->max) {
        size_t length_to_end = rb->max - head;
        memcpy(buffer, rb->buffer + head, length_to_end);
        memcpy(buffer + length_to_end, rb->buffer, amount - length_to_end);
    } else {
        memcpy(buffer, rb->buffer + head, amount);
    }
}

void ring_buffer_user_read(struct ring_buffer *rb, void *buffer, size_t amount) {
    ring_buffer_copy(rb, 0, buffer, amount);
    ring_buffer_advance(rb, amount);
}

void ring_buffer_write(struct ring_buffer *rb, const void *buffer, size_t amount) {
    ring_buffer_user_write(rb, buffer, amount);
}

void ring_buffer_user_write(struct ring_buffer *rb, const void *buffer, size_t amount) {
    if (rb->tail + amount > rb->max) {
        size_t length_to_end = rb->max - rb->tail;
        memcpy(rb->buffer + rb->tail, buffer, length_to_end);
        memcpy(rb->buffer, buffer + length_to_end, amount - length_to_end);
    } else {
        memcpy(rb->buffer + rb->tail, buffer, amount);
    }

    rb->tail += amount;
    rb->tail %= rb->max;
    rb->full = rb->head == rb->tail;
}
