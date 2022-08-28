#pragma once

#include <di/concepts/constructible_from.h>

namespace di::concepts {
template<typename T>
concept MoveConstructible = ConstructibleFrom<T, T&&>;
}
