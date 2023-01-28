#pragma once

#include <di/concepts/destructible.h>

namespace di::concepts {
template<typename T>
concept TriviallyDestructible = Destructible<T> && __has_trivial_destructor(T);
}
