#pragma once

#include <di/meta/type_constant.h>

namespace di::meta {
namespace detail {
    template<typename T>
    struct RemoveFunctionQualifiersHelper;

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...)> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) const> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) volatile> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) const volatile> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) &> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) const &> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) volatile &> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) const volatile &> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) &&> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) const &&> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) volatile &&> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) const volatile &&> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) noexcept> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) const noexcept> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) volatile noexcept> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) const volatile noexcept> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) & noexcept> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) const & noexcept> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) volatile & noexcept> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) const volatile & noexcept> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) && noexcept> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) const && noexcept> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) volatile && noexcept> : TypeConstant<R(Args...)> {};

    template<typename R, typename... Args>
    struct RemoveFunctionQualifiersHelper<R(Args...) const volatile && noexcept> : TypeConstant<R(Args...)> {};
}

template<typename T>
using RemoveFunctionQualifiers = detail::RemoveFunctionQualifiersHelper<T>::Type;
}
