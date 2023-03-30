#pragma once

#include <di/concepts/same_as.h>
#include <di/concepts/unsigned_integral.h>
#include <di/function/invoke.h>
#include <di/meta/bool_constant.h>

namespace di::concepts {
template<typename T>
concept UniformRandomBitGenerator = Invocable<T&> && UnsignedIntegral<meta::InvokeResult<T&>> && requires {
    { T::min() } -> SameAs<meta::InvokeResult<T&>>;
    { T::max() } -> SameAs<meta::InvokeResult<T&>>;
    requires meta::BoolConstant<(T::min() < T::max())>::value;
};
}
