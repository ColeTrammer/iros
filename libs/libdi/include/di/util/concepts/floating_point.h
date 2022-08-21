#pragma once

#include <di/util/concepts/one_of.h>
#include <di/util/meta/remove_cv.h>

namespace di::util::concepts {
template<typename T>
concept FloatingPoint = OneOf < meta::RemoveCV<T>,
float, double, long double > ;
}
