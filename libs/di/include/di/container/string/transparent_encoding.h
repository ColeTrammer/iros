#pragma once

#include "di/container/iterator/iterator_base.h"
#include "di/container/string/encoding.h"
#include "di/container/types/contiguous_iterator_tag.h"
#include "di/types/integers.h"

namespace di::container::string {
struct TransparentIterator : IteratorBase<TransparentIterator, ContiguousIteratorTag, char, isize> {
    TransparentIterator() = default;

    constexpr explicit TransparentIterator(char const* data) : m_data(data) {}

    constexpr auto operator*() const -> char const& { return *m_data; }
    constexpr auto operator->() const -> char const* { return m_data; }

    constexpr explicit operator char const*() const { return m_data; }

    constexpr void advance_one() { ++m_data; }
    constexpr void back_one() { --m_data; }
    constexpr void advance_n(isize n) { m_data += n; }

private:
    constexpr friend auto operator==(TransparentIterator a, TransparentIterator b) -> bool {
        return a.m_data == b.m_data;
    }
    constexpr friend auto operator<=>(TransparentIterator a, TransparentIterator b) { return a.m_data <=> b.m_data; }

    constexpr friend auto operator-(TransparentIterator a, TransparentIterator b) { return a.m_data - b.m_data; }

    char const* m_data { nullptr };
};

class TransparentEncoding {
public:
    using CodeUnit = char;
    using CodePoint = char;
    using Iterator = TransparentIterator;

private:
    constexpr friend auto tag_invoke(types::Tag<encoding::universal>, InPlaceType<TransparentEncoding>) -> bool {
        return true;
    }
    constexpr friend auto tag_invoke(types::Tag<encoding::contiguous>, InPlaceType<TransparentEncoding>) -> bool {
        return true;
    }
    constexpr friend auto tag_invoke(types::Tag<encoding::null_terminated>, InPlaceType<TransparentEncoding>) -> bool {
        return true;
    }
};
}
