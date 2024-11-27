#pragma once

#include <di/types/size_t.h>
#ifndef DI_NO_USE_STD
#include <initializer_list>
#else
namespace std {
template<typename T>
class initializer_list {
public:
    using value_type = T;
    using reference = T&;
    using const_reference = T const&;
    using size_type = std::size_t;
    using iterator = T const*;
    using const_iterator = iterator;

    constexpr initializer_list() {}

    constexpr auto size() const -> size_type { return m_size; }
    constexpr auto begin() const -> iterator { return m_data; }
    constexpr auto end() const -> iterator { return m_data + m_size; }

private:
    // This private constructor is called by the compiler to created initializer_list objects.
    constexpr initializer_list(iterator data, size_type size) : m_data(data), m_size(size) {}

    iterator m_data { nullptr };
    size_type m_size { 0 };
};
}
#endif
