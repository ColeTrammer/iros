#pragma once

#include <stddef.h>

#if !defined(__is_libc) && !defined(__is_libk)
#include <initializer_list>
#else
namespace std {
template<typename T>
class initializer_list {
public:
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using size_type = size_t;
    using iterator = const T*;
    using const_iterator = const T*;

    constexpr initializer_list() {}

    constexpr size_t size() const { return m_size; }
    constexpr const T* begin() const { return m_data; }
    constexpr const T* end() const { return m_data + m_size; }

private:
    constexpr initializer_list(const T* data, size_t size) : m_data(data), m_size(size) {}

    const T* m_data { nullptr };
    size_t m_size { 0 };
};
}
#endif
