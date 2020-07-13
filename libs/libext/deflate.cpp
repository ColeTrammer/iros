#include <ext/deflate.h>
#include <liim/fixed_array.h>
#include <limits.h>

#define DEFLATE_DEBUG

namespace Ext {

enum CompressionType {
    None = 0,
    Fixed = 1,
    Dynamic = 2,
};

struct Symbol {
    uint16_t symbol;
    uint16_t encoded_length;
    uint16_t code;
};

struct TreeNode {
    union {
        struct {
            uint16_t left : 15;
            uint16_t __padding0 : 1;
            uint16_t right : 15;
            uint16_t __padding1 : 1;
        };
        struct {
            uint16_t symbol : 15;
            uint16_t is_initialized : 1;
            uint16_t __padding2 : 15;
            uint16_t is_symbol : 1;
        };
    };
};

static_assert(sizeof(TreeNode) == 4);

static constexpr size_t max_bits = 15;

static uint8_t code_length_alphabet_order_mapping[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };

static Maybe<uint16_t> read_from_stream(uint8_t* compressed_data, size_t compressed_data_length, size_t num_bits, size_t& bit_offset) {
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

static constexpr void build_huffman_tree(Symbol* symbols, size_t num_symbols, TreeNode* nodes, size_t max_nodes) {
    FixedArray<uint16_t, max_bits + 1> bl_count;
    for (size_t i = 0; i < bl_count.size(); i++) {
        bl_count[i] = 0;
    }

    for (size_t i = 0; i < num_symbols; i++) {
        size_t index = symbols[i].encoded_length;
        assert(index <= max_bits);
        bl_count[index]++;
    }

    FixedArray<uint16_t, max_bits + 1> next_code;
    uint16_t code = 0;
    bl_count[0] = 0;
    for (size_t bits = 1; bits <= max_bits; bits++) {
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
            if (!nodes[tree_index].is_initialized) {
                nodes[tree_index].left = node_count;
                nodes[tree_index].right = node_count + 1;
                nodes[tree_index].is_initialized = 1;
                node_count += 2;
                assert(node_count <= max_nodes);
            }

            tree_index = bit ? nodes[tree_index].left : nodes[tree_index].right;
        }

        nodes[tree_index].is_initialized = 1;
        nodes[tree_index].is_symbol = 1;
        nodes[tree_index].symbol = symbols[i].symbol;
    }
}

Maybe<Vector<uint8_t>> decompress_deflate_payload(uint8_t* compressed_data, size_t compressed_data_length) {
    size_t bit_offset = 0;

    auto get = [&](auto bits) -> Maybe<uint16_t> {
        return read_from_stream(compressed_data, compressed_data_length, bits, bit_offset);
    };

    auto decode = [&](TreeNode* tree) -> Maybe<uint16_t> {
        size_t tree_index = 0;
        for (;;) {
            if (tree[tree_index].is_symbol) {
                return tree[tree_index].symbol;
            }

            auto bit = get(1);
            if (!bit.has_value()) {
                return {};
            }

            tree_index = bit.value() ? tree[tree_index].left : tree[tree_index].right;
        }
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

    FixedArray<Symbol, 19> code_length_symbols(19);
    for (size_t i = 0; i < 19; i++) {
        code_length_symbols[i].encoded_length = 0;
        code_length_symbols[i].code = 0;
        code_length_symbols[i].symbol = i;
    }

    for (size_t i = 0; i < hclen.value() + 4UL; i++) {
        auto len = get(3);
        if (!len.has_value()) {
            return {};
        }

        auto index = code_length_alphabet_order_mapping[i];
        code_length_symbols[index].encoded_length = len.value();
    }

#ifdef DEFLATE_DEBUG
    for (size_t i = 0; i < code_length_symbols.size(); i++) {
        fprintf(stderr, "CLA: %2u:%u\n", code_length_symbols[i].symbol, code_length_symbols[i].encoded_length);
    }
#endif /* DEFLATE_DEBUG */

    FixedArray<TreeNode, 128> length_codes_tree;
    for (size_t i = 0; i < 128; i++) {
        length_codes_tree[i].left = 0;
        length_codes_tree[i].is_initialized = 0;
        length_codes_tree[i].right = 0;
        length_codes_tree[i].is_symbol = 0;
    }
    build_huffman_tree(code_length_symbols.array(), code_length_symbols.size(), length_codes_tree.array(), length_codes_tree.size());

    FixedArray<Symbol, 286> literal_symbols(hlit.value() + 257);
    uint16_t prev_code_length = 0;
    for (size_t i = 0; i < hlit.value() + 257UL;) {
        auto value = decode(length_codes_tree.array());
        if (!value.has_value()) {
            return {};
        }

        fprintf(stderr, "[%3lu] value: %u\n", i, value.value());

        assert(value.value() <= 18);
        if (value.value() == 16) {
            auto repetitions = get(2);
            if (!repetitions.has_value()) {
                return {};
            }

            for (size_t j = 0; j < repetitions.value() + 3UL; j++) {
                literal_symbols[i].encoded_length = prev_code_length;
                literal_symbols[i].symbol = i;
                i++;
            }
            continue;
        }

        if (value.value() == 17) {
            auto repetitions = get(3);
            if (!repetitions.has_value()) {
                return {};
            }

            for (size_t j = 0; j < repetitions.value() + 3UL; j++) {
                literal_symbols[i].encoded_length = 0;
                literal_symbols[i].symbol = i;
                i++;
            }
            prev_code_length = 0;
            continue;
        }

        if (value.value() == 18) {
            auto repetitions = get(7);
            if (!repetitions.has_value()) {
                return {};
            }

            for (size_t j = 0; j < repetitions.value() + 11UL; j++) {
                literal_symbols[i].encoded_length = 0;
                literal_symbols[i].symbol = i;
                i++;
            }
            prev_code_length = 0;
            continue;
        }

        literal_symbols[i].encoded_length = value.value();
        literal_symbols[i].symbol = i;
        i++;
        prev_code_length = value.value();
    }

#ifdef DEFLATE_DEBUG
    for (size_t i = 0; i < hlit.value() + 257UL; i++) {
        fprintf(stderr, "LLA: %3u:%u\n", literal_symbols[i].symbol, literal_symbols[i].encoded_length);
    }
#endif /* DEFLATE_DEBUG */

    return {};
}

}
