#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/util/bitset.h>

void init_bitset(struct bitset *bitset, void *data, size_t data_bytes, size_t bit_count) {
    bitset->data = data;
    bitset->data_bytes = data_bytes;
    bitset->bit_count = bit_count;
    bitset->owned = false;

    assert(bit_count <= data_bytes * CHAR_BIT);
}

void init_owned_bitset(struct bitset *bitset, void *data, size_t data_bytes, size_t bit_count) {
    init_bitset(bitset, data, data_bytes, bit_count);
    bitset->owned = true;
}

void kill_bitset(struct bitset *bitset) {
    if (bitset->owned) {
        free(bitset->data);
    }
    memset(bitset, 0, sizeof(*bitset));
}

static int bitset_offset(const struct bitset *bitset, size_t bit, size_t *word_offset, size_t *bit_in_word_offset) {
    if (bit >= bitset->bit_count) {
        return -1;
    }

    *word_offset = bit / ((sizeof(bitset_word_t) * CHAR_BIT));
    *bit_in_word_offset = bit % (sizeof(bitset_word_t) * CHAR_BIT);
    return 0;
}

bool bitset_get_bit(const struct bitset *bitset, size_t bit) {
    size_t word_offset = 0;
    size_t bit_in_word_offset = 0;
    assert(bitset_offset(bitset, bit, &word_offset, &bit_in_word_offset) == 0);

    bitset_word_t word = bitset->data[word_offset];
    word &= (1LU << bit_in_word_offset);
    return word >> bit_in_word_offset;
}

void bitset_set_bit(struct bitset *bitset, size_t bit) {
    size_t word_offset = 0;
    size_t bit_in_word_offset = 0;
    assert(bitset_offset(bitset, bit, &word_offset, &bit_in_word_offset) == 0);

    bitset->data[word_offset] |= (1LU << bit_in_word_offset);
}

void bitset_clear_bit(struct bitset *bitset, size_t bit) {
    size_t word_offset = 0;
    size_t bit_in_word_offset = 0;
    assert(bitset_offset(bitset, bit, &word_offset, &bit_in_word_offset) == 0);

    bitset->data[word_offset] &= ~(1LU << bit_in_word_offset);
}

void bitset_toggle_bit(struct bitset *bitset, size_t bit) {
    size_t word_offset = 0;
    size_t bit_in_word_offset = 0;
    assert(bitset_offset(bitset, bit, &word_offset, &bit_in_word_offset) == 0);

    bitset->data[word_offset] ^= (1LU << bit_in_word_offset);
}

void bitset_write_bit(struct bitset *bitset, size_t bit, bool value) {
    if (value) {
        bitset_set_bit(bitset, bit);
    } else {
        bitset_clear_bit(bitset, bit);
    }
}

void bitset_set_bit_sequence(struct bitset *bitset, size_t bit_start, size_t length) {
    for (size_t i = bit_start; i < bit_start + length; i++) {
        bitset_set_bit(bitset, i);
    }
}

void bitset_clear_bit_sequence(struct bitset *bitset, size_t bit_start, size_t length) {
    for (size_t i = bit_start; i < bit_start + length; i++) {
        bitset_clear_bit(bitset, i);
    }
}

void bitset_toggle_bit_sequence(struct bitset *bitset, size_t bit_start, size_t length) {
    for (size_t i = bit_start; i < bit_start + length; i++) {
        bitset_toggle_bit(bitset, i);
    }
}

void bitset_write_bit_sequence(struct bitset *bitset, size_t bit_start, size_t length, bool value) {
    for (size_t i = bit_start; i < bit_start + length; i++) {
        bitset_write_bit(bitset, i, value);
    }
}

// NOTE: this should only be called for a word that actually has at least 1 zero byte.
static size_t bitset_first_zero_bit(bitset_word_t word) {
    for (size_t bit_index = 0;; bit_index++) {
        if (!(word & (1LU << bit_index))) {
            return bit_index;
        }
    }

    assert(false);
}

int bitset_find_first_free_bit(const struct bitset *bitset, size_t *bit) {
    for (size_t word_index = 0; word_index < bitset->data_bytes / sizeof(bitset_word_t); word_index++) {
        bitset_word_t word = bitset->data[word_index];
        if (!~word) {
            continue;
        }

        *bit = word_index * sizeof(bitset_word_t) * CHAR_BIT + bitset_first_zero_bit(word);
        return 0;
    }
    return -1;
}

int bitset_find_first_free_bit_sequence(const struct bitset *bitset, size_t length, size_t *bit_start) {
    for (size_t word_index = 0; word_index < bitset->data_bytes / sizeof(bitset_word_t); word_index++) {
        bitset_word_t word = bitset->data[word_index];
        if (!~word) {
            continue;
        }

        size_t bit_index_start = word_index * sizeof(bitset_word_t) * CHAR_BIT;
        size_t bit_index_end = bit_index_start + sizeof(bitset_word_t) * CHAR_BIT;
        for (size_t bit_index = bit_index_start; bit_index <= bit_index_end; bit_index++) {
            size_t consecutive_bits;
            for (consecutive_bits = 0; consecutive_bits < length; consecutive_bits++) {
                size_t bit_index_to_check = bit_index + consecutive_bits;
                if (bit_index_to_check >= bitset->bit_count || bitset_get_bit(bitset, bit_index_to_check)) {
                    break;
                }
            }

            if (consecutive_bits == length) {
                *bit_start = bit_index;
                return 0;
            }
        }
    }
    return -1;
}

static __attribute__((used)) void bitset_test() {
    struct bitset bitset;
    char buffer[27] = { 0 };
    init_bitset(&bitset, buffer, sizeof(buffer), sizeof(buffer) * CHAR_BIT - 5);

    assert(!bitset_get_bit(&bitset, 0));
    assert(!bitset_get_bit(&bitset, 1));
    assert(!bitset_get_bit(&bitset, 4));
    assert(!bitset_get_bit(&bitset, 90));

    bitset_set_bit(&bitset, 5);
    assert(bitset_get_bit(&bitset, 5));

    bitset_set_bit_sequence(&bitset, 0, 5);
    assert(bitset_get_bit(&bitset, 4));
    assert(bitset_get_bit(&bitset, 0));

    size_t bit = 0;
    assert(bitset_find_first_free_bit(&bitset, &bit) == 0);
    assert(bit == 6);

    assert(bitset_find_first_free_bit_sequence(&bitset, 5, &bit) == 0);
    assert(bit == 6);

    bitset_clear_bit(&bitset, 5);
    assert(!bitset_get_bit(&bitset, 5));

    assert(bitset_find_first_free_bit_sequence(&bitset, 1000, &bit) == -1);

    kill_bitset(&bitset);
}
