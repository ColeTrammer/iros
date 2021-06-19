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

static uint8_t code_length_alphabet_order_mapping[DeflateDecoder::hclen_max] = { 16, 17, 18, 0, 8,  7, 9,  6, 10, 5,
                                                                                 11, 4,  12, 3, 13, 2, 14, 1, 15 };

static CompressedOffset compressed_length_codes[DeflateDecoder::hlit_max - DeflateDecoder::hlit_offset] = {
    { 3, 0 },  { 4, 0 },  { 5, 0 },  { 6, 0 },   { 7, 0 },   { 8, 0 },   { 9, 0 },   { 10, 0 },  { 11, 1 }, { 13, 1 },
    { 15, 1 }, { 17, 1 }, { 19, 2 }, { 23, 2 },  { 27, 2 },  { 31, 2 },  { 35, 3 },  { 43, 3 },  { 51, 3 }, { 59, 3 },
    { 67, 4 }, { 83, 4 }, { 99, 4 }, { 115, 4 }, { 131, 5 }, { 163, 5 }, { 195, 5 }, { 227, 5 }, { 258, 0 }
};

static CompressedOffset compressed_distance_codes[DeflateDecoder::hdist_max] = {
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

StreamResult DeflateDecoder::stream_data(uint8_t* compressed_data, size_t compressed_data_length) {
    auto get = [&](auto bits) -> Maybe<uint16_t> {
        if (!m_in_get) {
            m_in_get = true;
            m_bits_to_get = bits;
            m_get_buffer = 0;
        }

        size_t byte_offset = m_bit_offset / CHAR_BIT;
        for (; byte_offset < compressed_data_length && m_bits_to_get;) {
            uint8_t byte = compressed_data[byte_offset];
            byte &= (1U << (m_bit_offset % 8));
            if (byte) {
                m_get_buffer |= 1U << (bits - m_bits_to_get);
            }

            if (++m_bit_offset % 8 == 0) {
                byte_offset++;
            }
            m_bits_to_get--;
        }

        if (m_bits_to_get) {
            return {};
        }

        m_in_get = false;
        return { m_get_buffer };
    };

    auto decode = [&](const TreeNode* tree) -> Maybe<uint16_t> {
        for (;;) {
            if (tree[m_tree_index].is_symbol()) {
                auto ret = tree[m_tree_index].symbol();
                m_tree_index = 0;
                return ret;
            }

            auto bit = get(1);
            if (!bit.has_value()) {
                return {};
            }

            m_tree_index = bit.value() ? tree[m_tree_index].left() : tree[m_tree_index].right();
        }
    };

    m_bit_offset = 0;
    switch (m_state) {
        case State::OnBlockIsLast:
            goto GetBlockIsLast;
        case State::OnBlockCompressionMethod:
            goto GetBlockCompressionMethod;
        case State::OnNoCompressionLen:
            goto GetNoCompressionLen;
        case State::OnNoCompressionNLen:
            goto GetNoCompressionNLen;
        case State::OnNoCompressionData:
            goto GetNoCompressionData;
        case State::OnLiteral:
            goto GetLiteral;
        case State::OnLengthExtraBits:
            goto GetLengthExtraBits;
        case State::OnDistanceCode:
            goto GetDistanceCode;
        case State::OnDistanceExtraBits:
            goto GetDistanceExtraBits;
        case State::OnHLit:
            goto GetHLit;
        case State::OnHDist:
            goto GetHDist;
        case State::OnHCLen:
            goto GetHCLen;
        case State::OnCodeLengthSymbol:
            goto GetCodeLengthSymbol;
        case State::OnLiteralAndDistanceSymbols:
            goto GetLiteralAndDistanceSymbols;
        case State::OnCodeLength16:
            goto GetCodeLength16;
        case State::OnCodeLength17:
            goto GetCodeLength17;
        case State::OnCodeLength18:
            goto GetCodeLength18;
    }

GetBlockIsLast : {
    if (m_is_last_block) {
        return StreamResult::Success;
    }

    m_state = State::OnBlockIsLast;
    auto is_last_block = get(1);
    if (!is_last_block.has_value()) {
        return StreamResult::NeedsMoreInput;
    }
    m_is_last_block = is_last_block.value();
}

GetBlockCompressionMethod : {
    m_state = State::OnBlockCompressionMethod;
    auto compression_type = get(2);
    if (!compression_type.has_value()) {
        return {};
    }

    switch (compression_type.value()) {
        case CompressionType::None:
#ifdef DEFLATE_DEBUG
            fprintf(stderr, "No compression block\n");
#endif /* DEFLATE_DEBUG */
            goto GetNoCompressionLen;
        case CompressionType::Fixed:
            m_literal_tree = static_literal_table.array();
            m_distance_tree = static_distance_table.array();
#ifdef DEFLATE_DEBUG
            fprintf(stderr, "Fixed compression block\n");
#endif /* DEFLATE_DEBUG */
            goto GetLiteral;
        case CompressionType::Dynamic:
#ifdef DEFLATE_DEBUG
            fprintf(stderr, "Dynamic compression block\n");
#endif /* DEFLATE_DEBUG */
            goto GetHLit;
        default:
            return StreamResult::Error;
    }
}

GetNoCompressionLen : {
    m_state = State::OnNoCompressionLen;
    if (m_bit_offset % 8) {
        m_bit_offset += 8 - (m_bit_offset % 8);
    }

    auto len = get(16);
    if (!len.has_value()) {
        return StreamResult::NeedsMoreInput;
    }
    m_no_compression_len = len.value();
}

GetNoCompressionNLen : {
    m_state = State::OnNoCompressionNLen;
    auto nlen = get(16);
    if (!nlen.has_value()) {
        return StreamResult::NeedsMoreInput;
    }
}

    m_i = 0;

GetNoCompressionData : {
    m_state = State::OnNoCompressionData;
    for (; m_i < m_no_compression_len; m_i++) {
        auto byte = get(8);
        if (!byte.has_value()) {
            return StreamResult::NeedsMoreInput;
        }
        m_decompressed_data.add(byte.value());
    }
    goto GetBlockIsLast;
}

GetLiteral : {
    m_state = State::OnLiteral;
    auto value = decode(m_literal_tree);
    if (!value.has_value()) {
        return StreamResult::NeedsMoreInput;
    }

#ifdef DEFLATE_DEBUG
    fprintf(stderr, "DECODE: %3u ('%c')\n", value.value(), value.value());
#endif /* DEFLATE_DEBUG */

    if (value.value() == block_end_marker) {
        goto GetBlockIsLast;
    }

    if (value.value() < block_end_marker) {
        m_decompressed_data.add(value.value());
        goto GetLiteral;
    }

    m_length_code = value.value() - hlit_offset;
}

GetLengthExtraBits : {
    m_state = State::OnLengthExtraBits;
    auto descriptor = compressed_length_codes[m_length_code];
    auto extra_data = get(descriptor.extra_bits);
    if (!extra_data.has_value()) {
        return StreamResult::NeedsMoreInput;
    }

    m_length = descriptor.offset + extra_data.value();
}

GetDistanceCode : {
    m_state = State::OnDistanceCode;
    auto distance_code = decode(m_distance_tree);
    if (!distance_code.has_value()) {
        return StreamResult::NeedsMoreInput;
    }

#ifdef DEFLATE_DEBUG
    fprintf(stderr, "DECODE: %3u (distance)\n", distance_code.value());
#endif /* DEFLATE_DEBUG */

    m_distance_code = distance_code.value();
}

GetDistanceExtraBits : {
    m_state = State::OnDistanceExtraBits;
    auto dst_descriptor = compressed_distance_codes[m_distance_code];
    auto extra_distance = get(dst_descriptor.extra_bits);
    if (!extra_distance.has_value()) {
        return StreamResult::NeedsMoreInput;
    }

    auto distance = dst_descriptor.offset + extra_distance.value();
#ifdef DEFLATE_DEBUG
    fprintf(stderr, "length=%u distance=%u data_len=%u\n", m_length, distance, m_decompressed_data.size());
#endif /* DEFLATE_DEBUG */

    size_t current_index = m_decompressed_data.size();
    if (current_index < static_cast<size_t>(distance)) {
        return StreamResult::Error;
    }
    for (size_t i = 0; i < m_length; i++) {
        auto byte = m_decompressed_data[current_index - distance + i];
#ifdef DEFLATE_DEBUG
        fprintf(stderr, "COPY:   %3u ('%c')\n", byte, byte);
#endif /* DEFLATE_DEBUG */
        m_decompressed_data.add(byte);
    }
    goto GetLiteral;
}

GetHLit : {
    m_state = State::OnHLit;
    auto hlit = get(5);
    if (!hlit.has_value()) {
        return StreamResult::NeedsMoreInput;
    }
    m_hlit = hlit.value();
}

GetHDist : {
    m_state = State::OnHDist;
    auto hdist = get(5);
    if (!hdist.has_value()) {
        return StreamResult::NeedsMoreInput;
    }
    m_hdist = hdist.value();
}

GetHCLen : {
    m_state = State::OnHCLen;
    auto hclen = get(4);
    if (!hclen.has_value()) {
        return StreamResult::NeedsMoreInput;
    }
    m_hclen = hclen.value();
#ifdef DEFLATE_DEBUG
    fprintf(stderr, "hlit=%lu hdist=%lu hclen=%lu\n", m_hlit + hlit_offset, m_hdist + hdist_offset, hclen.value() + hclen_offset);
#endif /* DEFLATE_DEBUG */
}

    for (size_t i = 0; i < m_code_length_symbols.size(); i++) {
        m_code_length_symbols[i].encoded_length = 0;
        m_code_length_symbols[i].code = 0;
        m_code_length_symbols[i].symbol = i;
    }
    m_i = 0;

GetCodeLengthSymbol : {
    m_state = State::OnCodeLengthSymbol;
    for (; m_i < m_hclen + hclen_offset; m_i++) {
        auto len = get(3);
        if (!len.has_value()) {
            return StreamResult::NeedsMoreInput;
        }

        auto index = code_length_alphabet_order_mapping[m_i];
        m_code_length_symbols[index].encoded_length = len.value();
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
}

    m_literal_and_distance_symbols.resize(m_hlit + hlit_offset + m_hdist + hdist_offset);
    for (size_t i = 0; i < m_hlit + hlit_offset; i++) {
        m_literal_and_distance_symbols[i].symbol = i;
    }
    for (size_t i = 0; i < m_hdist + hdist_offset; i++) {
        m_literal_and_distance_symbols[m_hlit + hlit_offset + i].symbol = i;
    }
    m_i = 0;

GetLiteralAndDistanceSymbols : {
    m_state = State::OnLiteralAndDistanceSymbols;

    if (m_i > m_literal_and_distance_symbols.size()) {
        return StreamResult::Error;
    }

    if (m_i == m_literal_and_distance_symbols.size()) {
#ifdef DEFLATE_DEBUG
        for (size_t i = 0; i < m_hlit + hlit_offset; i++) {
            fprintf(stderr, "LLA: %3u:%u\n", m_literal_and_distance_symbols[i].symbol, m_literal_and_distance_symbols[i].encoded_length);
        }
        for (size_t i = 0; i < m_hdist + hdist_offset; i++) {
            auto index = m_hlit + hlit_offset + i;
            fprintf(stderr, "DTA: %3u:%u\n", m_literal_and_distance_symbols[index].symbol,
                    m_literal_and_distance_symbols[index].encoded_length);
        }
#endif /* DEFLATE_DEBUG */

        for (size_t i = 0; i < m_hlit + hlit_offset; i++) {
            m_dynamic_literal_tree[i].m_left = 0;
            m_dynamic_literal_tree[i].m_right = 0;
        }
        build_huffman_tree(m_literal_and_distance_symbols.array(), m_hlit + hlit_offset, m_dynamic_literal_tree.array(),
                           m_dynamic_literal_tree.size());

        for (size_t i = 0; i < m_hdist + hdist_offset; i++) {
            m_dynamic_distance_tree[i].m_left = 0;
            m_dynamic_distance_tree[i].m_right = 0;
        }
        build_huffman_tree(m_literal_and_distance_symbols.array() + m_hlit + hlit_offset, m_hdist + hdist_offset,
                           m_dynamic_distance_tree.array(), m_dynamic_distance_tree.size());

        m_literal_tree = m_dynamic_literal_tree.array();
        m_distance_tree = m_dynamic_distance_tree.array();
        goto GetLiteral;
    }

    auto value = decode(m_length_codes_tree.array());
    if (!value.has_value()) {
        return StreamResult::NeedsMoreInput;
    }

#ifdef DEFLATE_DEBUG
    fprintf(stderr, "DECODE: %3u (code length)\n", value.value());
#endif /* DEFLATE_DEBUG */

    if (value.value() > 18) {
        return StreamResult::Error;
    }

    if (value.value() == 16) {
        goto GetCodeLength16;
    }

    if (value.value() == 17) {
        goto GetCodeLength17;
    }

    if (value.value() == 18) {
        goto GetCodeLength18;
    }

    m_literal_and_distance_symbols[m_i++].encoded_length = value.value();
    m_prev_code_length = value.value();
    goto GetLiteralAndDistanceSymbols;
}

GetCodeLength16 : {
    m_state = State::OnCodeLength16;
    auto repetitions = get(2);
    if (!repetitions.has_value()) {
        return StreamResult::NeedsMoreInput;
    }

    for (size_t j = 0; j < repetitions.value() + 3UL; j++) {
        m_literal_and_distance_symbols[m_i++].encoded_length = m_prev_code_length;
    }
    goto GetLiteralAndDistanceSymbols;
}

GetCodeLength17 : {
    m_state = State::OnCodeLength17;
    auto repetitions = get(3);
    if (!repetitions.has_value()) {
        return StreamResult::NeedsMoreInput;
    }

    for (size_t j = 0; j < repetitions.value() + 3UL; j++) {
        m_literal_and_distance_symbols[m_i++].encoded_length = 0;
    }
    m_prev_code_length = 0;
    goto GetLiteralAndDistanceSymbols;
}

GetCodeLength18 : {
    m_state = State::OnCodeLength18;
    auto repetitions = get(7);
    if (!repetitions.has_value()) {
        return StreamResult::NeedsMoreInput;
    }

    for (size_t j = 0; j < repetitions.value() + 11UL; j++) {
        m_literal_and_distance_symbols[m_i++].encoded_length = 0;
    }
    m_prev_code_length = 0;
    goto GetLiteralAndDistanceSymbols;
}
}

DeflateEncoder::DeflateEncoder(ByteWriter& writer) : m_encoder(encode()), m_writer(writer) {}

DeflateEncoder::~DeflateEncoder() {}

StreamResult DeflateEncoder::stream_data(Span<const uint8_t> input, StreamFlushMode mode) {
    m_reader.set_data(input);
    m_flush_mode = mode;
    return m_encoder();
}

Generator<StreamResult> DeflateEncoder::write_bits(uint32_t bits, uint8_t bit_count) {
    for (uint8_t i = 0; i < bit_count;) {
        if (!m_writer.write_bit(!!(bits & (1U << i)))) {
            co_yield StreamResult::NeedsMoreInput;
            continue;
        }
        i++;
    }
}

Generator<StreamResult> DeflateEncoder::write_bytes(const uint8_t* bytes, size_t byte_count) {
    for (size_t i = 0; i < byte_count;) {
        if (!m_writer.write_byte(bytes[i])) {
            co_yield StreamResult::NeedsMoreInput;
            continue;
        }
        i++;
    }
}

Generator<StreamResult> DeflateEncoder::encode() {
    for (;;) {
        uint16_t byte_count = INT16_MAX;
        uint16_t byte_count_complement = 0;

        bool last_block = false;
        if (m_reader.bytes_remaining() <= INT16_MAX) {
            byte_count = m_reader.bytes_remaining();
            byte_count_complement = ~byte_count;
            last_block = m_flush_mode == StreamFlushMode::StreamFlush;
        }

        co_yield write_bits(last_block, 1);
        co_yield write_bits(0b00, 2);

        uint16_t header[2] = { byte_count, byte_count_complement };
        co_yield write_bytes((const uint8_t*) header, sizeof(header));

        co_yield write_bytes(m_reader.data() + m_reader.byte_offset(), byte_count);
        m_reader.set_byte_offset(m_reader.byte_offset() + byte_count);

        if (last_block) {
            co_yield StreamResult::Success;
            co_return;
        }

        co_yield StreamResult::NeedsMoreInput;
    }
}
}
