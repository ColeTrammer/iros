#pragma once

#include <di/vocab/error/status_code.h>

namespace di::vocab {
template<typename T, typename U>
constexpr auto operator==(StatusCode<T> const& a, StatusCode<U> const& b) -> bool {
    return a.equivalent(b);
}

template<typename T, typename U>
requires(concepts::ConvertibleToAnyStatusCode<U const&>)
constexpr auto operator==(StatusCode<T> const& a, U const& b) -> bool {
    return a.equivalent(into_status_code(b));
}

template<typename T, typename U>
requires(concepts::ConvertibleToAnyStatusCode<T const&>)
constexpr auto operator==(T const& a, U const& b) -> bool {
    return into_status_code(a).equivalent(b);
}
}
