#pragma once

#include <di/meta/core.h>

namespace di::meta {
namespace detail {
    template<typename T, typename U>
    struct LikeHelper : TypeConstant<U> {};

    template<typename T, typename U>
    struct LikeHelper<T const, U> : TypeConstant<U const> {};

    template<typename T, typename U>
    struct LikeHelper<T&, U> : TypeConstant<U&> {};

    template<typename T, typename U>
    struct LikeHelper<T const&, U> : TypeConstant<U const&> {};

    template<typename T, typename U>
    struct LikeHelper<T&&, U> : TypeConstant<U&&> {};

    template<typename T, typename U>
    struct LikeHelper<T const&&, U> : TypeConstant<U const&&> {};
}

template<typename T, typename U>
using Like = detail::LikeHelper<T, U>::Type;
}
