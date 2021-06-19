#pragma once

#include <liim/byte_buffer.h>
#include <liim/maybe.h>
#include <liim/utilities.h>

namespace LIIM {
class ByteReader {
public:
    ByteReader() {}
    ByteReader(Span<const uint8_t> data) : m_data(data) {}
    ByteReader(const ByteBuffer& buffer) : m_data({ buffer.data(), buffer.size() }) {}

    const uint8_t* data() const { return m_data.data(); }

    size_t bytes_total() const { return m_data.size(); }
    size_t byte_offset() const { return m_byte_offset; }
    size_t bytes_remaining() const { return m_data.size() - byte_offset(); }
    bool finished() const { return bytes_remaining() == 0; }

    void reset() { m_byte_offset = 0; }

    void advance(size_t offset) {
        m_byte_offset += offset;
        assert(byte_offset() <= bytes_total());
    }

    void set_data(Span<const uint8_t> data) {
        reset();
        m_data = move(data);
    }

    void set_data(const ByteBuffer& buffer) {
        reset();
        m_data = { buffer.data(), buffer.size() };
    }

    Maybe<uint8_t> next_byte() {
        if (m_byte_offset >= m_data.size()) {
            return {};
        }
        return data()[m_byte_offset++];
    }

    void set_byte_offset(size_t byte_offset) {
        assert(byte_offset <= m_data.size());
        m_byte_offset = byte_offset;
    }

private:
    Span<const uint8_t> m_data;
    size_t m_byte_offset { 0 };
};

class ByteWriter {
public:
    ByteWriter(size_t capacity = 0) : m_buffer(capacity) {}

    ByteBuffer& buffer() { return m_buffer; }
    const ByteBuffer& buffer() const { return m_buffer; }

    size_t buffer_size() const { return m_buffer.size(); }
    size_t buffer_capacity() const { return m_buffer.capacity(); }

    uint8_t* data() { return m_buffer.data(); }
    const uint8_t* data() const { return m_buffer.data(); }

    void reset() {
        m_byte_offset = 0;
        m_bit_offset = 0;
        m_buffer.set_size(0);
    }

    bool write_bit(bool bit) {
        if (m_bit_offset == 8) {
            if (!commit_bits()) {
                return false;
            }
        }

        if (bit) {
            m_bit_buffer |= (1U << m_bit_offset);
        } else {
            m_bit_buffer &= ~(1U << m_bit_offset);
        }

        m_bit_offset++;
        return true;
    }

    bool write_byte(uint8_t byte) {
        if (m_bit_offset > 0) {
            if (!commit_bits()) {
                return false;
            }
        }

        return write_byte_impl(byte);
    }

private:
    bool commit_bits() {
        assert(m_bit_offset > 0);
        if (!write_byte_impl(m_bit_buffer)) {
            return false;
        }
        m_bit_offset = 0;
        m_bit_buffer = 0;
        return true;
    }

    bool write_byte_impl(uint8_t byte) {
        maybe_increase_capacity();
        buffer().set_size(m_byte_offset + 1);

        data()[m_byte_offset++] = byte;
        return true;
    }

    void maybe_increase_capacity() {
        if (m_byte_offset >= buffer_capacity()) {
            buffer().ensure_capacity(max(0x1000LU, 2 * buffer_capacity()));
        }
    }

    ByteBuffer m_buffer;
    size_t m_byte_offset { 0 };
    size_t m_bit_offset { 0 };
    uint8_t m_bit_buffer { 0 };
};

template<typename T>
static inline Span<const uint8_t> as_readonly_bytes(const T& value) {
    return { reinterpret_cast<const uint8_t*>(&value), sizeof(value) };
}

template<typename T>
static inline Span<uint8_t> as_writable_bytes(T& value) {
    return { reinterpret_cast<uint8_t*>(&value), sizeof(value) };
}
}

using LIIM::as_readonly_bytes;
using LIIM::as_writable_bytes;
using LIIM::ByteReader;
using LIIM::ByteWriter;
