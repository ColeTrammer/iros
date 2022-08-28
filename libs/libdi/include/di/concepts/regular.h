#pragma once

#include <di/concepts/equality_comparable.h>
#include <di/concepts/semiregular.h>

namespace di::concepts {
template<typename T>
concept Regular = Semiregular<T> && EqualityComparable<T>;
}
