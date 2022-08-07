#pragma once

#include <liim/container/producer/iterator_container.h>
#include <liim/container/strings/encoding.h>
#include <liim/span.h>

namespace LIIM::Container::Strings {
struct AsciiEncoding {
    using CodeUnitType = char;
    using CodePointType = char;
    using Iterator = char* const;

    constexpr static bool is_valid(Span<char const>) { return true; }
    constexpr static auto code_point_iterators(Span<char const> data) { return iterator_container(data.begin(), data.end()); }
    constexpr static bool is_valid_byte_offset(Span<char const>, size_t) { return true; }
};
}
