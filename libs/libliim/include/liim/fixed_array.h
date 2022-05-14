#pragma once

#include <assert.h>
#include <liim/span.h>
#include <stddef.h>

#if !defined(__is_libc) && !defined(__is_libk)
#include <initializer_list>
#endif

namespace LIIM {
template<typename T, size_t max_elements>
class FixedArray {
public:
    constexpr FixedArray(size_t size = max_elements) : m_size(size), m_array() { assert(size <= max_elements); }

#if !defined(__is_libc) && !defined(__is_libk)
    constexpr FixedArray(std::initializer_list<T> elements) : m_size(elements.size()), m_array() {
        size_t i = 0;
        for (auto it = elements.begin(); it != elements.end(); ++it, i++) {
            m_array[i] = *it;
        }
    }
#endif

    constexpr T& operator[](size_t index) {
        assert(index < m_size);
        return m_array[index];
    }

    constexpr const T& operator[](size_t index) const {
        assert(index < m_size);
        return m_array[index];
    }

    constexpr size_t size() const { return m_size; }

    constexpr T* array() { return m_array; }
    constexpr const T* array() const { return m_array; }

    constexpr void resize(size_t new_size) {
        m_size = new_size;
        assert(m_size <= max_elements);
    }

    constexpr T* begin() { return &m_array[0]; }
    constexpr T* end() { return &m_array[m_size]; }
    constexpr const T* begin() const { return &m_array[0]; }
    constexpr const T* end() const { return &m_array[m_size]; }
    constexpr const T* cbegin() const { return &m_array[0]; }
    constexpr const T* cend() const { return &m_array[m_size]; }

    constexpr Span<T> span() { return { begin(), size() }; }
    constexpr Span<const T> span() const { return { begin(), size() }; }

private:
    size_t m_size { 0 };
    T m_array[max_elements];
};

template<class T, class... U>
FixedArray(T, U...) -> FixedArray<T, 1 + sizeof...(U)>;
}

using LIIM::FixedArray;
