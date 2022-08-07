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
    typename T::CodeUnitType;
    typename T::CodePointType;
    typename T::Iterator;

    { T::is_valid(declval<Span<typename T::CodeUnitType const>>()) } -> SameAs<bool>;
    { T::code_point_iterators(declval<Span<typename T::CodeUnitType const>>()) } -> Container;
    { T::is_valid_byte_offset(declval<Span<typename T::CodeUnitType const>>(), declval<size_t>()) } -> SameAs<bool>;
};

struct AssumeProperlyEncoded {};

template<Encoding Enc>
using EncodingCodeUnitType = Enc::CodeUnitType;

template<Encoding Enc>
using EncodingCodePointType = Enc::CodePointType;

template<Encoding Enc>
using EncodingIteratorType = Enc::Iterator;
}
