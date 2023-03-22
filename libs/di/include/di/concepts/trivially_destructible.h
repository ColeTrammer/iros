#pragma once

#include <di/concepts/destructible.h>

namespace di::concepts {
#ifdef __clang__
template<typename T>
concept TriviallyDestructible = __is_trivially_destructible(T);
#else
template<typename T>
concept TriviallyDestructible = Destructible<T> && __has_trivial_destructor(T);
#endif
}
