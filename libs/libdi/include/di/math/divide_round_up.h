#pragma once

#include <di/concepts/integer.h>
#include <di/meta/type_identity.h>

namespace di::math {
template<concepts::Integer T>
constexpr T divide_round_up(T a, meta::TypeIdentity<T> b) {
    return (a + b - 1) / b;
}
}