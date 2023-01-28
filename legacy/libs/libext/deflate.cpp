#include <ext/deflate.h>
#include <liim/fixed_array.h>
#include <limits.h>

// #define DEFLATE_DEBUG

namespace Ext {
enum CompressionType {
    None = 0,
    Fixed = 1,
    Dynamic = 2,
};

struct CompressedOffset {
    uint16_t offset;
    uint8_t extra_bits;
};

static constexpr uint16_t block_end_marker = 256;

static constexpr uint8_t code_length_alphabet_order_mapping[DeflateDecoder::hclen_max] = {
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15,
};

static constexpr CompressedOffset compressed_length_codes[DeflateDecoder::hlit_max - DeflateDecoder::hlit_offset] = {
    { 3, 0 },  { 4, 0 },  { 5, 0 },  { 6, 0 },   { 7, 0 },   { 8, 0 },   { 9, 0 },   { 10, 0 },  { 11, 1 }, { 13, 1 },
    { 15, 1 }, { 17, 1 }, { 19, 2 }, { 23, 2 },  { 27, 2 },  { 31, 2 },  { 35, 3 },  { 43, 3 },  { 51, 3 }, { 59, 3 },
    { 67, 4 }, { 83, 4 }, { 99, 4 }, { 115, 4 }, { 131, 5 }, { 163, 5 }, { 195, 5 }, { 227, 5 }, { 258, 0 }
};

static constexpr CompressedOffset compressed_distance_codes[DeflateDecoder::hdist_max] = {
    { 1, 0 },     { 2, 0 },     { 3, 0 },     { 4, 0 },      { 5, 1 },      { 7, 1 },     { 9, 2 },     { 13, 2 },
    { 17, 3 },    { 25, 3 },    { 33, 4 },    { 49, 4 },     { 65, 5 },     { 97, 5 },    { 129, 6 },   { 193, 6 },
    { 257, 7 },   { 385, 7 },   { 513, 8 },   { 769, 8 },    { 1025, 9 },   { 1537, 9 },  { 2049, 10 }, { 3073, 10 },
    { 4097, 11 }, { 6145, 11 }, { 8193, 12 }, { 12289, 12 }, { 16385, 13 }, { 24577, 13 }
};

static constexpr void build_huffman_tree(Symbol* symbols, size_t num_symbols, TreeNode* nodes, size_t max_nodes) {
    FixedArray<uint16_t, DeflateDecoder::max_bits + 1> bl_count;
    for (size_t i = 0; i < bl_count.size(); i++) {
        bl_count[i] = 0;
    }

    for (size_t i = 0; i < num_symbols; i++) {
        size_t index = symbols[i].encoded_length;
        assert(index <= DeflateDecoder::max_bits);
        bl_count[index]++;
    }

    FixedArray<uint16_t, DeflateDecoder::max_bits + 1> next_code;
    uint16_t code = 0;
    bl_count[0] = 0;
    for (size_t bits = 1; bits <= DeflateDecoder::max_bits; bits++) {
        code = (code + bl_count[bits - 1]) << 1;
        next_code[bits] = code;
    }

    for (size_t n = 0; n < num_symbols; n++) {
        size_t len = symbols[n].encoded_length;
        if (len != 0) {
            symbols[n].code = next_code[len]++;
        } else {
            symbols[n].code = 0;
        }
    }

    size_t node_count = 1;
    for (size_t i = 0; i < num_symbols; i++) {
        auto code = symbols[i].code;
        auto len = symbols[i].encoded_length;
        if (!len) {
            continue;
        }

        uint16_t tree_index = 0;
        for (uint8_t bit_index = len; bit_index > 0; bit_index--) {
            auto bit = code & (1U << (bit_index - 1U));
            if (!nodes[tree_index].is_initialized()) {
                nodes[tree_index].m_left = node_count;
                nodes[tree_index].m_right = (node_count + 1) | 0x8000U;
                node_count += 2;
                assert(node_count <= max_nodes);
            }

            tree_index = bit ? nodes[tree_index].left() : nodes[tree_index].right();
        }

        nodes[tree_index].m_left = symbols[i].symbol | 0x8000U;
        nodes[tree_index].m_right = 0x8000U;
    }
}

static constexpr decltype(auto) build_static_huffman_literals() {
    FixedArray<Symbol, DeflateDecoder::hlit_max + 2> literal_symbols;
    for (size_t i = 0; i < literal_symbols.size(); i++) {
        literal_symbols[i].code = 0;
        literal_symbols[i].symbol = i;
        literal_symbols[i].encoded_length = i <= 143 ? 8 : i <= 255 ? 9 : i <= 279 ? 7 : 8;
    }

    FixedArray<TreeNode, 575> tree;
    for (size_t i = 0; i < tree.size(); i++) {
        tree[i].m_left = 0;
        tree[i].m_right = 0;
    }

    build_huffman_tree(literal_symbols.array(), literal_symbols.size(), tree.array(), tree.size());
    return tree;
}

static constexpr decltype(auto) build_static_huffman_distances() {
    FixedArray<Symbol, DeflateDecoder::hdist_max> distance_symbols;
    for (size_t i = 0; i < distance_symbols.size(); i++) {
        distance_symbols[i].code = 0;
        distance_symbols[i].symbol = i;
        distance_symbols[i].encoded_length = 5;
    }

    FixedArray<TreeNode, 63> tree;
    for (size_t i = 0; i < tree.size(); i++) {
        tree[i].m_left = 0;
        tree[i].m_right = 0;
    }

    build_huffman_tree(distance_symbols.array(), distance_symbols.size(), tree.array(), tree.size());
    return tree;
}

constexpr auto static_literal_table = build_static_huffman_literals();
constexpr auto static_distance_table = build_static_huffman_distances();

DeflateDecoder::DeflateDecoder() : StreamDecoder(decode()) {}

DeflateDecoder::~DeflateDecoder() {}

Generator<StreamResult> DeflateDecoder::decode_symbol(const TreeNode* tree, uint16_t& value) {
    int tree_index = 0;
    for (;;) {
        if (tree[tree_index].is_symbol()) {
            value = tree[tree_index].symbol();
            co_return;
        }

        auto bit = reader().next_bit();
        if (!bit) {
            co_yield StreamResult::NeedsMoreInput;
            continue;
        }

        tree_index = bit.value() ? tree[tree_index].left() : tree[tree_index].right();
    }
}

Generator<StreamResult> DeflateDecoder::decode_no_compression() {
    uint16_t length;
    co_yield read_bytes(as_writable_bytes(length));

    uint16_t length_complement;
    co_yield read_bytes(as_writable_bytes(length_complement));

    if ((length | length_complement) != static_cast<uint16_t>(-1)) {
        co_yield StreamResult::Error;
        co_return;
    }

    for (uint32_t i = 0; i < length; i++) {
        uint8_t byte;
        co_yield read_bytes(as_writable_bytes(byte));
        co_yield write_bytes(as_readonly_bytes(byte));
        m_previously_decoded_data.force_add(byte);
    }
}

Generator<StreamResult> DeflateDecoder::decode_dynamic_symbols() {
    uint32_t hlit;
    co_yield read_bits(hlit, 5);

    uint32_t hdist;
    co_yield read_bits(hdist, 5);

    uint32_t hclen;
    co_yield read_bits(hclen, 4);
#ifdef DEFLATE_DEBUG
    fprintf(stderr, "hlit=%lu hdist=%lu hclen=%lu\n", hlit + hlit_offset, hdist + hdist_offset, hclen + hclen_offset);
#endif /* DEFLATE_DEBUG */

    for (size_t i = 0; i < m_code_length_symbols.size(); i++) {
        m_code_length_symbols[i].encoded_length = 0;
        m_code_length_symbols[i].code = 0;
        m_code_length_symbols[i].symbol = i;
    }

    for (size_t i = 0; i < hclen + hclen_offset; i++) {
        uint32_t len;
        co_yield read_bits(len, 3);

        auto index = code_length_alphabet_order_mapping[i];
        m_code_length_symbols[index].encoded_length = len;
    }

#ifdef DEFLATE_DEBUG
    for (size_t i = 0; i < m_code_length_symbols.size(); i++) {
        fprintf(stderr, "CLA: %3u:%u\n", m_code_length_symbols[i].symbol, m_code_length_symbols[i].encoded_length);
    }
#endif /* DEFLATE_DEBUG */

    for (size_t i = 0; i < m_length_codes_tree.size(); i++) {
        m_length_codes_tree[i].m_left = 0;
        m_length_codes_tree[i].m_right = 0;
    }
    build_huffman_tree(m_code_length_symbols.array(), m_code_length_symbols.size(), m_length_codes_tree.array(),
                       m_length_codes_tree.size());

    m_literal_and_distance_symbols.resize(hlit + hlit_offset + hdist + hdist_offset);
    for (size_t i = 0; i < hlit + hlit_offset; i++) {
        m_literal_and_distance_symbols[i].symbol = i;
    }
    for (size_t i = 0; i < hdist + hdist_offset; i++) {
        m_literal_and_distance_symbols[hlit + hlit_offset + i].symbol = i;
    }

    size_t i = 0;
    uint16_t prev_code_length = 0;
    while (i < m_literal_and_distance_symbols.size()) {
        uint16_t value;
        co_yield decode_symbol(m_length_codes_tree.array(), value);
#ifdef DEFLATE_DEBUG
        fprintf(stderr, "DECODE: %3u (code length)\n", value);
#endif /* DEFLATE_DEBUG */

        if (value > 18) {
            co_yield StreamResult::Error;
            co_return;
        }

        if (value == 16) {
            uint32_t repetitions;
            co_yield read_bits(repetitions, 2);

            for (size_t j = 0; j < repetitions + 3UL; j++) {
                m_literal_and_distance_symbols[i++].encoded_length = prev_code_length;
            }
            continue;
        }

        if (value == 17) {
            uint32_t repetitions;
            co_yield read_bits(repetitions, 3);

            for (size_t j = 0; j < repetitions + 3UL; j++) {
                m_literal_and_distance_symbols[i++].encoded_length = 0;
            }
            prev_code_length = 0;
            continue;
        }

        if (value == 18) {
            uint32_t repetitions;
            co_yield read_bits(repetitions, 7);

            for (size_t j = 0; j < repetitions + 11UL; j++) {
                m_literal_and_distance_symbols[i++].encoded_length = 0;
            }
            prev_code_length = 0;
            continue;
        }

        m_literal_and_distance_symbols[i++].encoded_length = value;
        prev_code_length = value;
    }

    if (i > m_literal_and_distance_symbols.size()) {
        co_yield StreamResult::Error;
        co_return;
    }

#ifdef DEFLATE_DEBUG
    for (size_t i = 0; i < hlit + hlit_offset; i++) {
        fprintf(stderr, "LLA: %3u:%u\n", m_literal_and_distance_symbols[i].symbol, m_literal_and_distance_symbols[i].encoded_length);
    }
    for (size_t i = 0; i < hdist + hdist_offset; i++) {
        auto index = hlit + hlit_offset + i;
        fprintf(stderr, "DTA: %3u:%u\n", m_literal_and_distance_symbols[index].symbol,
                m_literal_and_distance_symbols[index].encoded_length);
    }
#endif /* DEFLATE_DEBUG */

    for (size_t i = 0; i < hlit + hlit_offset; i++) {
        m_dynamic_literal_tree[i].m_left = 0;
        m_dynamic_literal_tree[i].m_right = 0;
    }
    build_huffman_tree(m_literal_and_distance_symbols.array(), hlit + hlit_offset, m_dynamic_literal_tree.array(),
                       m_dynamic_literal_tree.size());

    for (size_t i = 0; i < hdist + hdist_offset; i++) {
        m_dynamic_distance_tree[i].m_left = 0;
        m_dynamic_distance_tree[i].m_right = 0;
    }
    build_huffman_tree(m_literal_and_distance_symbols.array() + hlit + hlit_offset, hdist + hdist_offset, m_dynamic_distance_tree.array(),
                       m_dynamic_distance_tree.size());

    m_literal_tree = m_dynamic_literal_tree.array();
    m_distance_tree = m_dynamic_distance_tree.array();
}

Generator<StreamResult> DeflateDecoder::decode_with_compression() {
    for (;;) {
        uint16_t value;
        co_yield decode_symbol(m_literal_tree, value);
#ifdef DEFLATE_DEBUG
        fprintf(stderr, "DECODE: %3u ('%c')\n", value, value);
#endif /* DEFLATE_DEBUG */

        if (value == block_end_marker) {
            break;
        }

        if (value < block_end_marker) {
            uint8_t value_byte = static_cast<uint8_t>(value);
            co_yield write_bytes(as_readonly_bytes(value_byte));
            m_previously_decoded_data.force_add(value_byte);
            continue;
        }

        auto length_code = value - hlit_offset;
        auto descriptor = compressed_length_codes[length_code];

        uint32_t extra_data;
        co_yield read_bits(extra_data, descriptor.extra_bits);

        auto length = descriptor.offset + extra_data;

        uint16_t distance_code;
        co_yield decode_symbol(m_distance_tree, distance_code);
#ifdef DEFLATE_DEBUG
        fprintf(stderr, "DECODE: %3u (distance)\n", distance_code);
#endif /* DEFLATE_DEBUG */

        auto dst_descriptor = compressed_distance_codes[distance_code];

        uint32_t extra_distance;
        co_yield read_bits(extra_distance, dst_descriptor.extra_bits);

        auto distance = dst_descriptor.offset + extra_distance;
#ifdef DEFLATE_DEBUG
        fprintf(stderr, "length=%u distance=%u\n", length, distance);
#endif /* DEFLATE_DEBUG */

        size_t current_index = m_previously_decoded_data.size();
        if (current_index < static_cast<size_t>(distance)) {
            co_yield StreamResult::Error;
            co_return;
        }

        for (size_t i = 0; i < length; i++) {
            auto byte = m_previously_decoded_data.access_from_tail(distance);
            co_yield write_bytes(as_readonly_bytes(byte));
            m_previously_decoded_data.force_add(byte);
        }
    }
}

Generator<StreamResult> DeflateDecoder::decode() {
    for (;;) {
        uint32_t block_header;
        co_yield read_bits(block_header, 3);

        bool is_last_block = !!(block_header & 1);
        uint8_t compression_type = block_header >> 1;

        switch (compression_type) {
            case CompressionType::None: {
                co_yield decode_no_compression();
                break;
            }
            case CompressionType::Fixed: {
                m_literal_tree = static_literal_table.array();
                m_distance_tree = static_distance_table.array();
                co_yield decode_with_compression();
                break;
            }
            case CompressionType::Dynamic: {
                co_yield decode_dynamic_symbols();
                co_yield decode_with_compression();
                break;
            }
            default:
                co_yield StreamResult::Error;
                co_return;
        }

        if (is_last_block) {
            co_yield StreamResult::Success;
            co_return;
        }
    }
}

DeflateEncoder::DeflateEncoder() : StreamEncoder(encode()) {}

DeflateEncoder::~DeflateEncoder() {}

Generator<StreamResult> DeflateEncoder::encode() {
    for (;;) {
        uint16_t byte_count = INT16_MAX;
        uint16_t byte_count_complement = 0;

        bool last_block = false;
        if (reader().bytes_remaining() <= INT16_MAX) {
            byte_count = reader().bytes_remaining();
            byte_count_complement = ~byte_count;
            last_block = flush_mode() == StreamFlushMode::StreamFlush;
        }

        co_yield write_bits(last_block, 1);
        co_yield write_bits(0b00, 2);

        uint16_t header[2] = { byte_count, byte_count_complement };
        co_yield write_bytes(array_as_readonly_bytes(header, sizeof(header) / sizeof(header[0])));

        co_yield write_bytes({ reader().data() + reader().byte_offset(), byte_count });
        reader().set_byte_offset({ reader().byte_offset() + byte_count });

        if (last_block) {
            co_yield StreamResult::Success;
            co_return;
        }

        co_yield StreamResult::NeedsMoreInput;
    }
}
}
