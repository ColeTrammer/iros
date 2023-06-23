#pragma once

#include <di/concepts/language_function.h>
#include <di/meta/core.h>
#include <di/meta/language_function_return.h>

namespace di::meta {
template<typename R>
struct IsFunctionTo {
    template<typename... T>
    requires(sizeof...(T) == 1)
    using Invoke = Constexpr<concepts::LanguageFunction<meta::Front<meta::List<T...>>> &&
                             concepts::SameAs<R, LanguageFunctionReturn<meta::Front<meta::List<T...>>>>>;
};
}
