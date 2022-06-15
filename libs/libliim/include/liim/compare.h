#pragma once

#include <liim/utilities.h>

namespace LIIM {
namespace Detail {
    template<typename T, typename U>
    struct ThreeWayCompareResult {
        using Type = decltype(declval<T>() <=> declval<U>());
    };
}

template<typename T, typename U = T>
using ThreeWayCompareResult = Detail::ThreeWayCompareResult<T, U>::Type;

template<typename T, typename U = T>
concept EqualComparable = requires(const T& t, const U& u) {
    { t == u } -> SameAs<bool>;
};

template<typename T, typename U = T>
concept Comparable = requires(const T& t, const U& u) {
    t <=> u;
};

template<typename Comp, typename T>
concept ComparatorFor = requires(const Comp& comparator, const T& a, const T& b) {
    { comparator(a, b) } -> SameAs<bool>;
};

template<Comparable T>
struct Less {
    template<Comparable<T> U>
    constexpr bool operator()(const T& a, const U& b) const {
        return a < b;
    }
};

template<Comparable T>
struct Greater {
    template<Comparable<T> U>
    constexpr bool operator()(const T& a, const U& b) const {
        return a > b;
    }
};
}

using LIIM::Comparable;
using LIIM::ComparatorFor;
using LIIM::EqualComparable;
using LIIM::Greater;
using LIIM::Less;
using LIIM::ThreeWayCompareResult;
