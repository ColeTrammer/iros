#pragma once

#include <di/any/concepts/interface.h>
#include <di/any/concepts/method_callable_with.h>

namespace di::concepts {
namespace detail {
    template<typename T, typename I>
    struct ImplHelper : meta::FalseType {};

    template<typename T, typename... Methods>
    struct ImplHelper<T, meta::List<Methods...>>
        : meta::BoolConstant<Conjunction<MethodCallableWith<meta::Type<Methods>, T>...>> {};
}

template<typename T, typename Interface>
concept Impl = detail::ImplHelper<T, Interface>::value;
}