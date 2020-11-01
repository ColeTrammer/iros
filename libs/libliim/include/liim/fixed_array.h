#pragma once

#include <assert.h>
#include <stddef.h>

template<typename T, size_t max_elements>
class FixedArray {
public:
    constexpr FixedArray(size_t size = max_elements) : m_size(size), m_array() { assert(size <= max_elements); }

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

private:
    size_t m_size { 0 };
    T m_array[max_elements];
};
