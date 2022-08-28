#pragma once

#include <di/concepts/one_of.h>
#include <di/meta/remove_cv.h>

namespace di::concepts {
template<typename T>
concept FloatingPoint = OneOf<meta::RemoveCV<T>, float, double, long double>;
}
