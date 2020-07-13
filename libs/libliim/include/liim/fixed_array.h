#pragma once

#include <liim/utilities.h>

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

private:
    size_t m_size { 0 };
    T m_array[max_elements];
};
