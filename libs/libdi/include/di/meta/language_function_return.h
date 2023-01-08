#pragma once

#include <di/concepts/language_function.h>
#include <di/meta/type_constant.h>

namespace di::meta {
namespace detail {
    template<typename T>
    struct LanguageFunctionReturnHelper;

    template<typename R, typename... Args>
    struct LanguageFunctionReturnHelper<R(Args...)> : TypeConstant<R> {};
}

template<concepts::LanguageFunction Fun>
using LanguageFunctionReturn = detail::LanguageFunctionReturnHelper<Fun>::Type;
}