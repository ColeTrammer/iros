#pragma once

#include <liim/container/algorithm/contains.h>
#include <liim/container/algorithm/ends_with.h>
#include <liim/container/algorithm/equal.h>
#include <liim/container/algorithm/find_first_not_of.h>
#include <liim/container/algorithm/find_first_of.h>
#include <liim/container/algorithm/find_last_not_of.h>
#include <liim/container/algorithm/find_last_of.h>
#include <liim/container/algorithm/find_last_subrange.h>
#include <liim/container/algorithm/find_subrange.h>
#include <liim/container/algorithm/lexographic_compare.h>
#include <liim/container/algorithm/size.h>
#include <liim/container/algorithm/starts_with.h>
#include <liim/container/strings/encoding.h>
#include <liim/container/vector/vector_algorithm.h>
#include <liim/option.h>
#include <liim/span.h>

namespace LIIM::Container::Strings::Algorithm {
namespace Detail {
    template<Encoding Enc, Vector::ReadonlyVectorStorageOf<EncodingCodeUnit<Enc>> Str>
    constexpr Vector::VectorStorageConstIterator<Str> into_vector_iterator(Enc, Str& string, EncodingIterator<Enc> iterator) {
        return Vector::Algorithm::begin(string) + Enc::iterator_code_unit_offset(string, iterator);
    }
}

template<Encoding Enc>
constexpr Option<EncodingCodePoint<Enc>> front(Enc, Span<EncodingCodeUnit<Enc> const> data) {
    auto [start, end] = Enc::code_point_iterators(data);
    if (start == end) {
        return None {};
    }
    return *start;
}

template<Encoding Enc>
constexpr Option<EncodingCodePoint<Enc>> back(Enc, Span<EncodingCodeUnit<Enc> const> data) {
    auto [start, end] = Enc::code_point_iterators(data);
    if (start == end) {
        return None {};
    }
    return *--end;
}

template<Encoding Enc>
constexpr bool empty(Enc, Span<EncodingCodeUnit<Enc> const> data) {
    auto [start, end] = Enc::code_point_iterators(data);
    return start == end;
}

template<Encoding Enc>
constexpr size_t size_in_code_points(Enc, Span<EncodingCodeUnit<Enc> const> data) {
    return Alg::size(Enc::code_point_iterators(data));
}

template<Encoding Enc>
constexpr bool equal(Enc, Span<EncodingCodeUnit<Enc> const> a, Span<EncodingCodeUnit<Enc> const> b) {
    return Alg::equal(Enc::code_point_iterators(a), Enc::code_point_iterators(b));
}

template<Encoding Enc>
constexpr std::strong_ordering compare(Enc, Span<EncodingCodeUnit<Enc> const> a, Span<EncodingCodeUnit<Enc> const> b) {
    return Alg::lexographic_compare(Enc::code_point_iterators(a), Enc::code_point_iterators(b));
}

struct StartsWithFunction {
    template<Encoding Enc>
    constexpr auto operator()(Enc, Span<EncodingCodeUnit<Enc> const> haystack, Span<EncodingCodeUnit<Enc> const> needle) const {
        return Alg::starts_with(Enc::code_point_iterators(haystack), Enc::code_point_iterators(needle));
    }

    template<Encoding Enc>
    constexpr auto operator()(Enc encoding, Span<EncodingCodeUnit<Enc> const> haystack, EncodingCodePoint<Enc> needle) const {
        return Algorithm::front(encoding, haystack) == needle;
    }
};

constexpr inline auto starts_with = StartsWithFunction {};

struct EndsWithFunction {
    template<Encoding Enc>
    constexpr auto operator()(Enc, Span<EncodingCodeUnit<Enc> const> haystack, Span<EncodingCodeUnit<Enc> const> needle) const {
        return Alg::ends_with(Enc::code_point_iterators(haystack), Enc::code_point_iterators(needle));
    }

    template<Encoding Enc>
    constexpr auto operator()(Enc encoding, Span<EncodingCodeUnit<Enc> const> haystack, EncodingCodePoint<Enc> needle) const {
        return Algorithm::back(encoding, haystack) == needle;
    }
};

constexpr inline auto ends_with = EndsWithFunction {};

struct ContainsFunction {
    template<Encoding Enc>
    constexpr auto operator()(Enc, Span<EncodingCodeUnit<Enc> const> haystack, Span<EncodingCodeUnit<Enc> const> needle) const {
        return Alg::contains_subrange(Enc::code_point_iterators(haystack), Enc::code_point_iterators(needle));
    }

    template<Encoding Enc>
    constexpr auto operator()(Enc, Span<EncodingCodeUnit<Enc> const> haystack, EncodingCodePoint<Enc> needle) const {
        return Alg::contains(Enc::code_point_iterators(haystack), needle);
    }
};

constexpr inline auto contains = ContainsFunction {};

struct FindFunction {
    template<Encoding Enc>
    constexpr auto operator()(Enc, Span<EncodingCodeUnit<Enc> const> haystack, Span<EncodingCodeUnit<Enc> const> needle) const {
        return Alg::find_subrange(Enc::code_point_iterators(haystack), Enc::code_point_iterators(needle));
    }

    template<Encoding Enc>
    constexpr auto operator()(Enc, Span<EncodingCodeUnit<Enc> const> haystack, EncodingCodePoint<Enc> needle) const {
        return Alg::find(Enc::code_point_iterators(haystack), needle);
    }
};

constexpr inline auto find = FindFunction {};

struct RFindFunction {
    template<Encoding Enc>
    constexpr auto operator()(Enc, Span<EncodingCodeUnit<Enc> const> haystack, Span<EncodingCodeUnit<Enc> const> needle) const {
        return Alg::find_last_subrange(Enc::code_point_iterators(haystack), Enc::code_point_iterators(needle));
    }

    template<Encoding Enc>
    constexpr auto operator()(Enc, Span<EncodingCodeUnit<Enc> const> haystack, EncodingCodePoint<Enc> needle) const {
        return Alg::find_last(Enc::code_point_iterators(haystack), needle);
    }
};

constexpr inline auto rfind = RFindFunction {};

struct FindFirstOfFunction {
    template<Encoding Enc>
    constexpr auto operator()(Enc, Span<EncodingCodeUnit<Enc> const> haystack, Span<EncodingCodeUnit<Enc> const> needle) const {
        return Alg::find_first_of(Enc::code_point_iterators(haystack), Enc::code_point_iterators(needle));
    }
};

constexpr inline auto find_first_of = FindFirstOfFunction {};

struct FindLastOf {
    template<Encoding Enc>
    constexpr auto operator()(Enc, Span<EncodingCodeUnit<Enc> const> haystack, Span<EncodingCodeUnit<Enc> const> needle) const {
        return Alg::find_last_of(Enc::code_point_iterators(haystack), Enc::code_point_iterators(needle));
    }
};

constexpr inline auto find_last_of = FindLastOf {};

struct FindFirstNotOf {
    template<Encoding Enc>
    constexpr auto operator()(Enc, Span<EncodingCodeUnit<Enc> const> haystack, Span<EncodingCodeUnit<Enc> const> needle) const {
        return Alg::find_first_not_of(Enc::code_point_iterators(haystack), Enc::code_point_iterators(needle));
    }
};

constexpr inline auto find_first_not_of = FindFirstNotOf {};

struct FindLastNotOf {
    template<Encoding Enc>
    constexpr auto operator()(Enc, Span<EncodingCodeUnit<Enc> const> haystack, Span<EncodingCodeUnit<Enc> const> needle) const {
        return Alg::find_last_not_of(Enc::code_point_iterators(haystack), Enc::code_point_iterators(needle));
    }
};

constexpr inline auto find_last_not_of = FindLastNotOf {};

template<Encoding Enc, Vector::VectorStorageOf<EncodingCodeUnit<Enc>> Str>
constexpr void clear(Enc, Str& string) {
    return Vector::Algorithm::clear(string);
}

template<Encoding Enc, Vector::VectorStorageOf<EncodingCodeUnit<Enc>> Str>
constexpr void append(Enc, Str& string, Span<EncodingCodeUnit<Enc> const> data) {
    return Vector::Algorithm::append_container(string, data);
}

template<Encoding Enc, Vector::VectorStorageOf<EncodingCodeUnit<Enc>> Str>
constexpr void push_back(Enc, Str& string, EncodingCodePoint<Enc> code_point) {
    return Vector::Algorithm::append_container(string, Enc::code_point_to_code_units(code_point));
}

template<Encoding Enc, Vector::VectorStorageOf<EncodingCodeUnit<Enc>> Str>
constexpr Option<EncodingCodePoint<Enc>> pop_back(Enc encoding, Str& string) {
    auto [begin, end] = Enc::code_point_iterators(string.span());
    if (begin == end) {
        return None {};
    }

    auto start_to_erase = end;
    --start_to_erase;
    auto result = *start_to_erase;

    Vector::Algorithm::erase(string, Detail::into_vector_iterator(encoding, string, start_to_erase), Vector::Algorithm::end(string));
    return result;
}

template<Encoding Enc, Vector::VectorStorageOf<EncodingCodeUnit<Enc>> Str>
constexpr void erase(Enc encoding, Str& string, EncodingIterator<Enc> begin, EncodingIterator<Enc> end) {
    return Vector::Algorithm::erase(string, Detail::into_vector_iterator(encoding, string, begin),
                                    Detail::into_vector_iterator(encoding, string, end));
}

template<Encoding Enc, Vector::VectorStorageOf<EncodingCodeUnit<Enc>> Str>
constexpr void erase(Enc encoding, Str& string, EncodingIterator<Enc> position) {
    auto end = position;
    end++;
    return Algorithm::erase(encoding, string, position, end);
}

template<Encoding Enc, Vector::VectorStorageOf<EncodingCodeUnit<Enc>> Str>
constexpr void insert(Enc encoding, Str& string, EncodingIterator<Enc> position, EncodingCodePoint<Enc> code_point) {
    return Vector::Algorithm::insert_container(string, Detail::into_vector_iterator(encoding, string, position),
                                               Enc::code_point_to_code_units(code_point));
}

template<Encoding Enc, Vector::VectorStorageOf<EncodingCodeUnit<Enc>> Str>
constexpr void insert(Enc encoding, Str& string, EncodingIterator<Enc> position, Span<EncodingCodeUnit<Enc> const> data) {
    return Vector::Algorithm::insert_container(string, Detail::into_vector_iterator(encoding, string, position), data);
}

template<Encoding Enc, Vector::VectorStorageOf<EncodingCodeUnit<Enc>> Str>
constexpr void replace(Enc encoding, Str& string, EncodingIterator<Enc> begin, EncodingIterator<Enc> end,
                       Span<EncodingCodeUnit<Enc> const> replacement) {
    return Vector::Algorithm::replace(string, Detail::into_vector_iterator(encoding, string, begin),
                                      Detail::into_vector_iterator(encoding, string, end), replacement);
}
}
