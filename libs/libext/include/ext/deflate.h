#pragma once

#include <liim/byte_buffer.h>
#include <liim/byte_io.h>
#include <liim/fixed_array.h>
#include <liim/generator.h>
#include <liim/maybe.h>
#include <liim/string.h>
#include <liim/vector.h>
#include <stdint.h>
#include <sys/types.h>

namespace Ext {

enum class StreamResult {
    Success,
    Error,
    NeedsMoreData,
};

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

class DeflateDecoder {
public:
    static constexpr size_t max_bits = 15;
    static constexpr size_t hlit_offset = 257;
    static constexpr size_t hlit_max = 286;
    static constexpr size_t hdist_offset = 1;
    static constexpr size_t hdist_max = 32;
    static constexpr size_t hclen_offset = 4;
    static constexpr size_t hclen_max = 19;

    StreamResult stream_data(uint8_t* compressed_data, size_t compressed_data_length);

    Vector<uint8_t>& decompressed_data() { return m_decompressed_data; }
    const Vector<uint8_t>& decompressed_data() const { return m_decompressed_data; }

    size_t last_byte_offset() const { return (m_bit_offset + 7LU) / 8LU; }

private:
    enum class State {
        OnBlockIsLast,
        OnBlockCompressionMethod,
        OnNoCompressionLen,
        OnNoCompressionNLen,
        OnNoCompressionData,
        OnLiteral,
        OnLengthExtraBits,
        OnDistanceCode,
        OnDistanceExtraBits,
        OnHLit,
        OnHDist,
        OnHCLen,
        OnCodeLengthSymbol,
        OnLiteralAndDistanceSymbols,
        OnCodeLength16,
        OnCodeLength17,
        OnCodeLength18,
    };

    Vector<uint8_t> m_decompressed_data;
    size_t m_bit_offset { 0 };
    State m_state { State::OnBlockIsLast };
    uint16_t m_no_compression_len { 0 };
    uint16_t m_tree_index { 0 };
    uint16_t m_length_code { 0 };
    uint16_t m_length { 0 };
    uint16_t m_distance_code { 0 };
    uint8_t m_hlit { 0 };
    uint8_t m_hdist { 0 };
    uint8_t m_hclen { 0 };
    size_t m_i { 0 };
    uint16_t m_prev_code_length { 0 };
    uint16_t m_bits_to_get { 0 };
    uint16_t m_get_buffer { 0 };
    FixedArray<Symbol, hclen_max> m_code_length_symbols;
    FixedArray<TreeNode, 128> m_length_codes_tree;
    FixedArray<Symbol, hlit_max + hdist_max> m_literal_and_distance_symbols;
    FixedArray<TreeNode, 8192> m_dynamic_literal_tree;
    FixedArray<TreeNode, 512> m_dynamic_distance_tree;
    const TreeNode* m_literal_tree { nullptr };
    const TreeNode* m_distance_tree { nullptr };
    bool m_is_last_block { false };
    bool m_in_get { false };
};

class ZLibStreamDecoder {
public:
    StreamResult stream_data(uint8_t* data, size_t data_len);

    Vector<uint8_t>& decompressed_data() { return m_deflate_stream.decompressed_data(); }
    const Vector<uint8_t>& decompressed_data() const { return m_deflate_stream.decompressed_data(); }

private:
    enum class State {
        OnCMF,
        OnFLG,
        OnDICTID,
        OnDeflateData,
        OnAdler32,
    };

    DeflateDecoder m_deflate_stream;
    State m_state { State::OnCMF };
    size_t m_offset { 0 };
    uint8_t m_compression_method { 0 };
    uint8_t m_flags { 0 };
    bool m_in_get { false };
    uint8_t m_bytes_to_get { 0 };
    uint32_t m_get_buffer { 0 };
};

struct GZipData {
    Maybe<Vector<uint8_t>> extra_data;
    Maybe<String> name;
    Maybe<String> comment;
    time_t time_last_modified;
    Vector<uint8_t> decompressed_data;
};

enum class FlushMode {
    NoFlush,
    BlockFlush,
    StreamFlush,
};

class DeflateEncoder {
public:
    DeflateEncoder(ByteWriter& writer);
    ~DeflateEncoder();

    StreamResult stream_data(Span<const uint8_t> input, FlushMode mode);

private:
    Generator<StreamResult> encode();

    Generator<StreamResult> write_bits(uint32_t bits, uint8_t bit_count);
    Generator<StreamResult> write_bytes(const uint8_t* bytes, size_t byte_count);

    Generator<StreamResult> m_encoder;
    FlushMode m_flush_mode { FlushMode::NoFlush };
    ByteReader m_reader;
    ByteWriter& m_writer;
};

Maybe<GZipData> read_gzip_path(const String& path);

}
