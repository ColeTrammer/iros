#pragma once

#include <assert.h>
#include <stddef.h>

namespace LIIM {
template<typename T>
class Span {
public:
    constexpr Span() {}
    constexpr Span(T* data, size_t size) : m_data(data), m_size(size) {}

    constexpr T* begin() const { return m_data; }
    constexpr T* end() const { return m_data + m_size; }

    constexpr T& operator[](size_t index) const {
        assert(index < m_size);
        return m_data[index];
    }

    constexpr bool empty() const { return m_size == 0; }
    constexpr size_t size() const { return m_size; }
    constexpr size_t size_in_bytes() const { return sizeof(T) * m_size; }

    constexpr T* data() const { return m_data; }

    constexpr Span first(size_t count) const {
        assert(count <= m_size);
        return Span(m_data, count);
    }
    constexpr Span last(size_t count) const {
        assert(count <= m_size);
        return Span(m_data + (m_size - count), count);
    }
    constexpr Span subspan(size_t index, size_t count) const {
        assert(index + count <= m_size);
        return Span(m_data + index, count);
    }

private:
    T* m_data { nullptr };
    size_t m_size { 0 };
};
}

using LIIM::Span;
