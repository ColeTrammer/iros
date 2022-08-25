#pragma once

#include <di/util/concepts/trivially_assignable_from.h>

namespace di::util::concepts {
template<typename T>
concept TriviallyMoveAssignable = TriviallyAssignableFrom<T&, T&&>;
}
