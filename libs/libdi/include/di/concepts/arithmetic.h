#pragma once

#include <di/concepts/floating_point.h>
#include <di/concepts/integral.h>

namespace di::concepts {
template<typename T>
concept Arithmetic = Integral<T> || FloatingPoint<T>;
}
