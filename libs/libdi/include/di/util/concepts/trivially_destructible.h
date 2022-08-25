#pragma once

#include <di/util/concepts/destructible.h>

namespace di::util::concepts {
template<typename T>
concept TriviallyDestructible = Destructible<T> && __has_trivial_destructor(T);
}
