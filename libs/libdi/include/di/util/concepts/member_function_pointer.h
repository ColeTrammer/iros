#pragma once

#include <di/util/concepts/language_function.h>
#include <di/util/meta/bool_constant.h>
#include <di/util/meta/false_type.h>
#include <di/util/meta/remove_cv.h>

namespace di::util::concepts {
namespace detail {
    template<typename T>
    struct MemberFunctionPointer : meta::FalseType {};

    template<typename T, typename U>
    struct MemberFunctionPointer<T U::*> : meta::BoolConstant<LanguageFunction<T>> {};
}

template<typename T>
concept MemberFunctionPointer = detail::MemberFunctionPointer<meta::RemoveCV<T>>::value;
}
