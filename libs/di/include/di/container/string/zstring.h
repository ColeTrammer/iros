#pragma once

#include <di/container/iterator/prelude.h>

namespace di::container {
template<di::concepts::OneOf<char, unsigned char, wchar_t> T>
class ZStringImpl : public di::meta::EnableBorrowedContainer<ZStringImpl<T>> {
private:
    struct Iterator : di::container::IteratorExtension<Iterator, T const*, T> {
    private:
        using Base = di::container::IteratorExtension<Iterator, T const*, T>;

    public:
        Iterator() = default;

        constexpr explicit Iterator(T const* base) : Base(base) {}

        constexpr T const& operator*() const { return *this->base(); }

    private:
        constexpr friend bool operator==(Iterator const& a, di::DefaultSentinel) { return *a == 0; }
    };

public:
    constexpr explicit ZStringImpl(T const* data) : m_data(data) {}

    constexpr T const* data() const { return m_data; }

    constexpr Iterator begin() const { return Iterator(m_data); }
    constexpr auto end() const { return di::default_sentinel; }

private:
    T const* m_data { nullptr };
};

using ZString = ZStringImpl<char>;
using ZUString = ZStringImpl<unsigned char>;
using ZWString = ZStringImpl<wchar_t>;
}