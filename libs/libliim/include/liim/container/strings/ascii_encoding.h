#pragma once

#include <liim/container/iterator/wrapper_iterator.h>
#include <liim/container/producer/iterator_container.h>
#include <liim/container/producer/single.h>
#include <liim/container/strings/encoding.h>
#include <liim/span.h>

namespace LIIM::Container::Strings {
class AsciiIterator : public ContinuousIteratorAdapter<AsciiIterator> {
public:
    using ValueType = char;

    constexpr ValueType operator*() const { return m_data[this->index()]; }

    constexpr size_t current_code_unit_offset() const { return this->index(); }

private:
    constexpr AsciiIterator(Span<char const> data, size_t index) : ContinuousIteratorAdapter<AsciiIterator>(index), m_data(data) {}

    friend class AsciiEncoding;

    Span<char const> m_data;
};

struct AsciiEncoding {
    using CodeUnit = char;
    using CodePoint = char;
    using Iterator = AsciiIterator;

    constexpr static bool is_valid(Span<char const>) { return true; }
    constexpr static auto code_point_iterators(Span<char const> data) {
        return iterator_container(AsciiIterator(data, 0), AsciiIterator(data, data.size()));
    }
    constexpr static bool is_valid_byte_offset(Span<char const> data, size_t offset) { return offset <= data.size(); }
    constexpr static Option<Iterator> iterator_at_offset(Span<char const> data, size_t offset) {
        if (!is_valid_byte_offset(data, offset)) {
            return None {};
        }
        return AsciiIterator(data, offset);
    }
    constexpr static auto code_point_to_code_units(char code_point) { return single(code_point); }
};
}
