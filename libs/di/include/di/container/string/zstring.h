#pragma once

#include <di/container/iterator/prelude.h>

namespace di::container {
template<di::concepts::OneOf<char, unsigned char, wchar_t, char const, unsigned char const, wchar_t const> T>
class ZStringImpl : public di::meta::EnableBorrowedContainer<ZStringImpl<T>> {
private:
    struct Iterator : di::container::IteratorExtension<Iterator, T*, T> {
    private:
        using Base = di::container::IteratorExtension<Iterator, T*, T>;

    public:
        Iterator() = default;

        constexpr explicit Iterator(T* base) : Base(base) {}

        constexpr T& operator*() const { return *this->base(); }

    private:
        constexpr friend bool operator==(Iterator const& a, di::DefaultSentinel) { return *a == 0; }
    };

public:
    constexpr explicit ZStringImpl(T* data) : m_data(data) {}

    constexpr T* data() const { return m_data; }

    constexpr Iterator begin() const { return Iterator(m_data); }
    constexpr auto end() const { return di::default_sentinel; }

private:
    T* m_data { nullptr };
};

using ZCString = ZStringImpl<char const>;
using ZCUString = ZStringImpl<unsigned char const>;
using ZCWString = ZStringImpl<wchar_t const>;
using ZString = ZStringImpl<char>;
using ZUString = ZStringImpl<unsigned char>;
using ZWString = ZStringImpl<wchar_t>;
}
