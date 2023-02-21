#pragma once

#include <di/vocab/error/status_code.h>

namespace di::vocab {
template<typename T, typename U>
constexpr bool operator==(StatusCode<T> const& a, StatusCode<U> const& b) {
    return a.equivalent(b);
}

template<typename T, typename U>
requires(concepts::ConvertibleToAnyStatusCode<U const&>)
constexpr bool operator==(StatusCode<T> const& a, U const& b) {
    return a.equivalent(into_status_code(b));
}

template<typename T, typename U>
requires(concepts::ConvertibleToAnyStatusCode<T const&>)
constexpr bool operator==(T const& a, U const& b) {
    return into_status_code(a).equivalent(b);
}
}
