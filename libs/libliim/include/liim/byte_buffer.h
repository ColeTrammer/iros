#pragma once

#include <assert.h>
#include <liim/span.h>
#include <liim/utilities.h>
#include <stdint.h>

namespace LIIM {
class ByteBuffer {
public:
    explicit ByteBuffer(size_t capacity = 0) : m_data(nullptr) { ensure_capacity(capacity); }
    ByteBuffer(const ByteBuffer& other) = delete;
    ByteBuffer(ByteBuffer&& other)
        : m_data(exchange(other.m_data, nullptr))
        , m_data_size(exchange(other.m_data_size, 0))
        , m_data_capacity(exchange(other.m_data_capacity, 0)) {}

    ~ByteBuffer() { clear(); }

    ByteBuffer& operator=(const ByteBuffer& other) = delete;
    ByteBuffer& operator=(ByteBuffer&& other) {
        if (this != &other) {
            ByteBuffer temp(move(other));
            swap(temp);
        }

        return *this;
    }

    uint8_t* data() { return m_data; }
    const uint8_t* data() const { return m_data; }

    bool empty() const { return m_data_size == 0; }
    size_t size() const { return m_data_size; }
    size_t capacity() const { return m_data_capacity; }

    uint8_t* begin() { return data(); }
    uint8_t* end() { return data() + size(); }

    const uint8_t* begin() const { return data(); }
    const uint8_t* end() const { return data() + size(); }

    uint8_t& operator[](size_t index) {
        assert(index < size());
        return m_data[index];
    }
    uint8_t operator[](size_t index) const {
        assert(index < size());
        return m_data[index];
    }

    void set_size(size_t size) {
        assert(size < capacity());
        m_data_size = size;
    }

    void clear() {
        if (m_data) {
            free(m_data);
            m_data = nullptr;
        }
        m_data_size = 0;
        m_data_capacity = 0;
    }

    void ensure_capacity(size_t new_capacity) {
        if (capacity() < new_capacity) {
            m_data = static_cast<uint8_t*>(realloc(m_data, new_capacity));
        }
        m_data_capacity = max(m_data_capacity, new_capacity);
    }

    size_t append_fixed(const Span<const uint8_t>& bytes) {
        size_t index = 0;
        for (index = 0; size() < capacity() && index < bytes.size(); index++) {
            m_data[m_data_size++] = bytes[index];
        }
        return index;
    }

    void append(const Span<const uint8_t>& bytes) {
        size_t new_size = size() + bytes.size();
        if (new_size > m_data_capacity) {
            ensure_capacity(max(capacity() * 2, new_size));
        }
        append_fixed(bytes);
    }

    void swap(ByteBuffer& other) {
        LIIM::swap(this->m_data, other.m_data);
        LIIM::swap(this->m_data_size, other.m_data_size);
        LIIM::swap(this->m_data_capacity, other.m_data_capacity);
    }

private:
    uint8_t* m_data { nullptr };
    size_t m_data_size { 0 };
    size_t m_data_capacity { 0 };
};

inline void swap(ByteBuffer& a, ByteBuffer& b) {
    a.swap(b);
}
}

using LIIM::ByteBuffer;
