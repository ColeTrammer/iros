#pragma once

#include "di/function/invoke.h"
#include "di/meta/constexpr.h"
#include "di/meta/core.h"
#include "di/meta/language.h"

namespace di::concepts {
template<typename T>
concept UniformRandomBitGenerator = Invocable<T&> && UnsignedIntegral<meta::InvokeResult<T&>> && requires {
    { T::min() } -> SameAs<meta::InvokeResult<T&>>;
    { T::max() } -> SameAs<meta::InvokeResult<T&>>;
    requires meta::Constexpr<(T::min() < T::max())>::value;
};
}

namespace di {
using concepts::UniformRandomBitGenerator;
}
