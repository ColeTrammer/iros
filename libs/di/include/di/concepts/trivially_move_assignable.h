#pragma once

#include <di/concepts/trivially_assignable_from.h>

namespace di::concepts {
template<typename T>
concept TriviallyMoveAssignable = TriviallyAssignableFrom<T&, T&&>;
}
