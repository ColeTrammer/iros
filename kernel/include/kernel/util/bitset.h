#ifndef _KERNEL_UTIL_BITSET_H
#define _KERNEL_UTIL_BITSET_H 1

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef unsigned long bitset_word_t;

struct bitset {
    bitset_word_t *data;
    size_t data_bytes;
    size_t bit_count;
};

void init_bitset(struct bitset *bitset, void *data, size_t data_bytes, size_t bit_count);
void kill_bitset(struct bitset *bitset);

bool bitset_get_bit(const struct bitset *bitset, size_t bit);
void bitset_set_bit(struct bitset *bitset, size_t bit);
void bitset_clear_bit(struct bitset *bitset, size_t bit);
void bitset_toggle_bit(struct bitset *bitset, size_t bit);
void bitset_write_bit(struct bitset *bitset, size_t bit, bool value);

void bitset_set_bit_sequence(struct bitset *bitset, size_t bit_start, size_t length);
void bitset_clear_bit_sequence(struct bitset *bitset, size_t bit_start, size_t length);
void bitset_toggle_bit_sequence(struct bitset *bitset, size_t bit_start, size_t length);
void bitset_write_bit_sequence(struct bitset *bitset, size_t bit_start, size_t length, bool value);

int bitset_find_first_free_bit(const struct bitset *bitset, size_t *bit);
int bitset_find_first_free_bit_sequence(const struct bitset *bitset, size_t length, size_t *bit_start);

static inline size_t bitset_bit_count(const struct bitset *bitset) {
    return bitset->bit_count;
}

#endif /* _KERNEL_UTIL_BITSET_H */
