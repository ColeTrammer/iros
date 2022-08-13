#pragma once

#include <liim/container/concepts.h>
#include <liim/utilities.h>

namespace LIIM::Container::Strings {
template<Integral T>
struct EncodingUnit {
    T code_point { 0 };
    uint8_t encoded_byte_size { 0 };
};

template<typename T>
concept Encoding = requires {
    typename T::CodeUnit;
    typename T::CodePoint;
    typename T::Iterator;

    { T::is_valid(declval<Span<typename T::CodeUnit const>>()) } -> SameAs<bool>;
    { T::code_point_iterators(declval<Span<typename T::CodeUnit const>>()) } -> Container;
    { T::is_valid_byte_offset(declval<Span<typename T::CodeUnit const>>(), declval<size_t>()) } -> SameAs<bool>;
    { T::iterator_at_offset(declval<Span<typename T::CodeUnit const>>(), declval<size_t>()) } -> SameAs<Option<typename T::Iterator>>;
};

struct AssumeProperlyEncoded {};

template<Encoding Enc>
using EncodingCodeUnit = Enc::CodeUnit;

template<Encoding Enc>
using EncodingCodePoint = Enc::CodePoint;

template<Encoding Enc>
using EncodingIterator = Enc::Iterator;
}
