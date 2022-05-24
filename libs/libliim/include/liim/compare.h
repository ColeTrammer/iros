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
}

using LIIM::Comparable;
using LIIM::EqualComparable;
using LIIM::ThreeWayCompareResult;
