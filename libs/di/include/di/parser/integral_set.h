#pragma once

#include <di/concepts/integral.h>
#include <di/concepts/three_way_comparable_with.h>
#include <di/container/algorithm/any_of.h>
#include <di/function/unpack.h>
#include <di/meta/make_index_sequence.h>
#include <di/vocab/array/prelude.h>

namespace di::parser {
template<concepts::Integral T>
struct MatchOne {
    constexpr bool operator()(concepts::ThreeWayComparableWith<T> auto const& other) const { return value == other; }

    T value;
};

template<concepts::Integral T>
struct MatchRange {
    constexpr bool operator()(concepts::ThreeWayComparableWith<T> auto const& other) const {
        return lower <= other && other <= upper_inclusive;
    }

    T lower;
    T upper_inclusive;
};

template<concepts::Integral T, size_t N>
struct IntegralSet {
    constexpr bool operator()(concepts::ThreeWayComparableWith<T> auto const& other) const {
        return container::any_of(ranges, [&](auto const& range) {
            return range(other);
        });
    }

    Array<MatchRange<T>, N> ranges;
};

template<concepts::Integral T, size_t N>
struct InvertedIntegralSet {
    constexpr bool operator()(concepts::ThreeWayComparableWith<T> auto const& other) const { return !set(other); }

    IntegralSet<T, N> set;
};

template<concepts::Integral T>
constexpr auto operator~(MatchOne<T> value) {
    return InvertedIntegralSet<T, 1> { IntegralSet<T, 1> { MatchRange<T> { value.value, value.value } } };
}

template<concepts::Integral T>
constexpr auto operator~(MatchRange<T> value) {
    return InvertedIntegralSet<T, 1> { IntegralSet<T, 1> { value } };
}

template<concepts::Integral T, size_t N>
constexpr auto operator~(IntegralSet<T, N> value) {
    return InvertedIntegralSet<T, N> { value };
}

template<concepts::Integral T>
constexpr auto operator-(MatchOne<T> a, MatchOne<T> b) {
    return MatchRange<T> { a.value, b.value };
}

template<concepts::Integral T>
constexpr auto operator||(MatchOne<T> a, MatchOne<T> b) {
    return MatchRange<T> { a.value, a.value } || MatchRange<T> { b.value, b.value };
}

template<concepts::Integral T>
constexpr auto operator||(MatchOne<T> a, MatchRange<T> b) {
    return MatchRange<T> { a.value, a.value } || b;
}

template<concepts::Integral T>
constexpr auto operator||(MatchRange<T> a, MatchOne<T> b) {
    return a || MatchRange<T> { b.value, b.value };
}

template<concepts::Integral T>
constexpr auto operator||(MatchRange<T> a, MatchRange<T> b) {
    return IntegralSet<T, 1> { a } || IntegralSet<T, 1> { b };
}

template<concepts::Integral T, size_t N>
constexpr auto operator||(IntegralSet<T, N> a, MatchOne<T> b) {
    return a || MatchRange<T> { b.value, b.value };
}

template<concepts::Integral T, size_t N>
constexpr auto operator||(MatchOne<T> a, IntegralSet<T, N> b) {
    return MatchRange<T> { a.value, a.value } | b;
}

template<concepts::Integral T, size_t N>
constexpr auto operator||(IntegralSet<T, N> a, MatchRange<T> b) {
    return a || IntegralSet<T, 1> { b };
}

template<concepts::Integral T, size_t N>
constexpr auto operator||(MatchRange<T> a, IntegralSet<T, N> b) {
    return IntegralSet<T, 1> { a } || b;
}

template<concepts::Integral T, size_t N1, size_t N2>
constexpr auto operator||(IntegralSet<T, N1> a, IntegralSet<T, N2> b) {
    return function::unpack<meta::MakeIndexSequence<N1>>([&]<size_t... a_indices>(meta::ListV<a_indices...>) {
        return function::unpack<meta::MakeIndexSequence<N2>>([&]<size_t... b_indices>(meta::ListV<b_indices...>) {
            return IntegralSet<T, N1 + N2> {
                util::get<a_indices>(a.ranges)...,
                util::get<b_indices>(b.ranges)...,
            };
        });
    });
}
}

namespace di {
inline namespace literals {
    inline namespace integral_set_literals {

#define DI_DEFINE_INTEGRAL_OP(input_kind, output_kind, name)            \
    constexpr auto operator"" name(input_kind value) {                  \
        return parser::MatchOne<output_kind> { (output_kind) (value) }; \
    }

        DI_DEFINE_INTEGRAL_OP(char, char, _mc)
        DI_DEFINE_INTEGRAL_OP(char, c32, _m)
        DI_DEFINE_INTEGRAL_OP(c32, c32, _m)
        DI_DEFINE_INTEGRAL_OP(unsigned long long, unsigned long long, _mu)

#undef DI_DEFINE_INTEGRAL_OP
    }
}
}
