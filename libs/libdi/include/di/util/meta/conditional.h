#pragma once

#include <di/util/meta/type_constant.h>

namespace di::util::meta {
namespace detail {
    template<bool value, typename T, typename U>
    struct ConditionalHelper : TypeConstant<T> {};

    template<typename T, typename U>
    struct ConditionalHelper<false, T, U> : TypeConstant<U> {};
}

template<bool value, typename T, typename U>
using Conditional = detail::ConditionalHelper<value, T, U>::Type;
}
