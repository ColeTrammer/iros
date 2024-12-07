#pragma once

#include "di/meta/operations.h"
#include "di/util/declval.h"
#include "di/util/forward.h"

namespace di::concepts {
template<typename F, typename... Args>
concept Callable = requires(F&& function, Args&&... args) {
    { util::forward<F>(function)(util::forward<Args>(args)...) };
};
}

namespace di::meta {
template<typename F, typename... Args>
using CallResult = decltype(util::declval<F>()(util::declval<Args>()...));
}

namespace di::concepts {
template<typename F, typename T, typename... Args>
concept CallableTo = concepts::Callable<F, Args...> && ImplicitlyConvertibleTo<T, meta::CallResult<F, Args...>>;
}

namespace di {
using concepts::Callable;
using concepts::CallableTo;
using meta::CallResult;
}
