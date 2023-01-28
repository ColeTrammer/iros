#pragma once

#include <di/concepts/arithmetic.h>

namespace di::concepts {
template<typename T>
concept Signed = Arithmetic<T> && (T(-1) < T(0));
}
