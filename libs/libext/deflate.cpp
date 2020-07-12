#include <ext/deflate.h>
#include <liim/fixed_array.h>
#include <limits.h>

#define DEFLATE_DEBUG

namespace Ext {

enum CompressionType {
    None,
    Fixed,
    Dynamic,
};

static uint8_t code_length_alphabet_order_mapping[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };

Maybe<uint16_t> read_from_stream(uint8_t* compressed_data, size_t compressed_data_length, size_t num_bits, size_t& bit_offset) {
    uint16_t value = 0;

    size_t offset = bit_offset;
    size_t byte_offset = bit_offset / CHAR_BIT;
    offset -= byte_offset * CHAR_BIT;
    size_t i = 0;
    for (; byte_offset < compressed_data_length && i < num_bits; i++) {
        uint8_t byte = compressed_data[byte_offset];
        byte &= (1U << offset);
        if (byte) {
            value += 1U << i;
        }

        if (++offset == CHAR_BIT) {
            byte_offset++;
            offset = 0;
        }
    }

    if (i != num_bits) {
        return {};
    }

    bit_offset += num_bits;
    return { value };
}

Maybe<Vector<uint8_t>> decompress_deflate_payload(uint8_t* compressed_data, size_t compressed_data_length) {
    size_t bit_offset = 0;

    auto get = [&](auto bits) -> Maybe<uint16_t> {
        return read_from_stream(compressed_data, compressed_data_length, bits, bit_offset);
    };

    auto is_last_block = get(1);
    if (!is_last_block.value()) {
        return {};
    }
    auto compression_type = get(2);
    if (!compression_type.value()) {
        return {};
    }

    assert(is_last_block.value());
    assert(compression_type.value() == CompressionType::Dynamic);

    auto hlit = get(5);
    if (!hlit.has_value()) {
        return {};
    }
    auto hdist = get(5);
    if (!hdist.has_value()) {
        return {};
    }
    auto hclen = get(4);
    if (!hclen.has_value()) {
        return {};
    }

#ifdef DEFLATE_DEBUG
    fprintf(stderr, "hlit=%u hdist=%u hclen=%u\n", hlit.value() + 257, hdist.value() + 1, hclen.value() + 4);
#endif /* DEFLATE_DEBUG */

    FixedArray<uint8_t, 19> code_length_frequencies(hclen.value() + 4);
    for (size_t i = 0; i < hclen.value() + 4UL; i++) {
        auto len = get(3);
        if (!len.has_value()) {
            return {};
        }

        code_length_frequencies[i] = len.value();
    }

#ifdef DEFLATE_DEBUG
    for (size_t i = 0; i < code_length_frequencies.size(); i++) {
        fprintf(stderr, "CLA: %2u:%u\n", code_length_alphabet_order_mapping[i], code_length_frequencies[i]);
    }
#endif /* DEFLATE_DEBUG */

    return {};
}

}
