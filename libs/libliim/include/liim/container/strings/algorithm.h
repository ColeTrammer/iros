#pragma once

#include <liim/container/algorithm/contains.h>
#include <liim/container/algorithm/ends_with.h>
#include <liim/container/algorithm/equal.h>
#include <liim/container/algorithm/lexographic_compare.h>
#include <liim/container/algorithm/size.h>
#include <liim/container/algorithm/starts_with.h>
#include <liim/container/strings/encoding.h>
#include <liim/option.h>
#include <liim/span.h>

namespace LIIM::Container::Strings::Algorithm {
template<Encoding Enc>
constexpr Option<EncodingCodePointType<Enc>> front(Enc, Span<EncodingCodeUnitType<Enc> const> data) {
    auto [start, end] = Enc::code_point_iterators(data);
    if (start == end) {
        return None {};
    }
    return *start;
}

template<Encoding Enc>
constexpr Option<EncodingCodePointType<Enc>> back(Enc, Span<EncodingCodeUnitType<Enc> const> data) {
    auto [start, end] = Enc::code_point_iterators(data);
    if (start == end) {
        return None {};
    }
    return *--end;
}

template<Encoding Enc>
constexpr bool empty(Enc, Span<EncodingCodeUnitType<Enc> const> data) {
    auto [start, end] = Enc::code_point_iterators(data);
    return start == end;
}

template<Encoding Enc>
constexpr size_t size_in_code_points(Enc, Span<EncodingCodeUnitType<Enc> const> data) {
    return Alg::size(Enc::code_point_iterators(data));
}

template<Encoding Enc>
constexpr bool equal(Enc, Span<EncodingCodeUnitType<Enc> const> a, Span<EncodingCodeUnitType<Enc> const> b) {
    return Alg::equal(Enc::code_point_iterators(a), Enc::code_point_iterators(b));
}

template<Encoding Enc>
constexpr std::strong_ordering compare(Enc, Span<EncodingCodeUnitType<Enc> const> a, Span<EncodingCodeUnitType<Enc> const> b) {
    return Alg::lexographic_compare(Enc::code_point_iterators(a), Enc::code_point_iterators(b));
}

template<Encoding Enc>
constexpr bool starts_with(Enc, Span<EncodingCodeUnitType<Enc> const> a, Span<EncodingCodeUnitType<Enc> const> b) {
    return Alg::starts_with(Enc::code_point_iterators(a), Enc::code_point_iterators(b));
}

template<Encoding Enc>
constexpr bool ends_with(Enc, Span<EncodingCodeUnitType<Enc> const> a, Span<EncodingCodeUnitType<Enc> const> b) {
    return Alg::ends_with(Enc::code_point_iterators(a), Enc::code_point_iterators(b));
}

template<Encoding Enc>
constexpr bool contains(Enc, Span<EncodingCodeUnitType<Enc> const> a, Span<EncodingCodeUnitType<Enc> const> b) {
    return Alg::contains(Enc::code_point_iterators(a), Enc::code_point_iterators(b));
}

struct FindFunction {
    // template<Encoding Enc>
    // constexpr EncodingIteratorType<Enc> operator()(Span<EncodingCodeUnitType<Enc> const> haystack, Span<EncodingCodeUnitType<Enc> const>
    // needle,
    //                                                Option<EncodingIteratorType<Enc>> offset = {}) const {

    // }
};

constexpr inline auto find = FindFunction {};
}
