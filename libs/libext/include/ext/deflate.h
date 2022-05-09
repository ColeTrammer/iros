#pragma once

#include <ext/stream_decoder.h>
#include <ext/stream_encoder.h>
#include <liim/byte_buffer.h>
#include <liim/byte_io.h>
#include <liim/fixed_array.h>
#include <liim/generator.h>
#include <liim/option.h>
#include <liim/ring_buffer.h>
#include <liim/string.h>
#include <liim/vector.h>
#include <stdint.h>
#include <sys/types.h>

namespace Ext {
struct Symbol {
    uint16_t symbol;
    uint16_t encoded_length;
    uint16_t code;
};

struct TreeNode {
    constexpr bool is_symbol() const { return m_left & 0x8000U; }
    constexpr bool is_initialized() const { return m_right & 0x8000U; }

    constexpr uint16_t left() const { return m_left & 0x7FFFU; }
    constexpr uint16_t right() const { return m_right & 0x7FFFU; }

    constexpr uint16_t symbol() const { return left(); }

    uint16_t m_left;
    uint16_t m_right;
};

static_assert(sizeof(TreeNode) == 4);

class DeflateDecoder final : public StreamDecoder {
public:
    static constexpr size_t max_bits = 15;
    static constexpr size_t hlit_offset = 257;
    static constexpr size_t hlit_max = 286;
    static constexpr size_t hdist_offset = 1;
    static constexpr size_t hdist_max = 32;
    static constexpr size_t hclen_offset = 4;
    static constexpr size_t hclen_max = 19;

    DeflateDecoder();
    virtual ~DeflateDecoder() override;

private:
    Generator<StreamResult> decode();

    Generator<StreamResult> decode_symbol(const TreeNode* tree, uint16_t& value);

    Generator<StreamResult> decode_no_compression();
    Generator<StreamResult> decode_with_compression();
    Generator<StreamResult> decode_dynamic_symbols();

    RingBuffer<uint8_t, 32768> m_previously_decoded_data;
    FixedArray<Symbol, hclen_max> m_code_length_symbols;
    FixedArray<TreeNode, 128> m_length_codes_tree;
    FixedArray<Symbol, hlit_max + hdist_max> m_literal_and_distance_symbols;
    FixedArray<TreeNode, 8192> m_dynamic_literal_tree;
    FixedArray<TreeNode, 512> m_dynamic_distance_tree;
    const TreeNode* m_literal_tree;
    const TreeNode* m_distance_tree;
};

class DeflateEncoder final : public StreamEncoder {
public:
    DeflateEncoder();
    virtual ~DeflateEncoder() override;

private:
    Generator<StreamResult> encode();
};
}
