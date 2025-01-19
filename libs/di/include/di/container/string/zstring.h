#pragma once

#include "di/container/iterator/prelude.h"

namespace di::container {
template<di::concepts::OneOf<char, unsigned char, wchar_t, char const, unsigned char const, wchar_t const, c8, c8 const,
                             c16, c16 const, c32, c32 const>
             T>
class ZStringImpl : public di::meta::EnableBorrowedContainer<ZStringImpl<T>> {
private:
    struct Iterator : di::container::IteratorExtension<Iterator, T*, T> {
    private:
        using Base = di::container::IteratorExtension<Iterator, T*, T>;

    public:
        Iterator() = default;

        constexpr explicit Iterator(T* base) : Base(base) {}

        constexpr auto operator*() const -> T& { return *this->base(); }

    private:
        constexpr friend auto operator==(Iterator const& a, di::DefaultSentinel) -> bool { return *a == 0; }
    };

public:
    constexpr explicit ZStringImpl(T* data) : m_data(data) {}

    constexpr auto data() const -> T* { return m_data; }

    constexpr auto begin() const -> Iterator { return Iterator(m_data); }
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
using ZC8CString = ZStringImpl<c8 const>;
using ZC8String = ZStringImpl<c8>;
using ZC16CString = ZStringImpl<c16 const>;
using ZC16String = ZStringImpl<c16>;
using ZC32CString = ZStringImpl<c32 const>;
using ZC32String = ZStringImpl<c32>;
}

namespace di {
using container::ZC16CString;
using container::ZC16String;
using container::ZC32CString;
using container::ZC32String;
using container::ZC8CString;
using container::ZC8String;
using container::ZCString;
using container::ZCUString;
using container::ZCWString;
using container::ZString;
using container::ZUString;
using container::ZWString;
}
