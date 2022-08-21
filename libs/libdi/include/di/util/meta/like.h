#pragma once

#include <di/util/meta/type_constant.h>

namespace di::util::meta {
namespace detail {
    template<typename T, typename U>
    struct LikeHelper : TypeConstant<U> {};

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
