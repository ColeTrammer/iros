#pragma once

#include <di/util/concepts/floating_point.h>
#include <di/util/concepts/integral.h>

namespace di::util::concepts {
template<typename T>
concept Arithmetic = Integral<T> || FloatingPoint<T>;
}
