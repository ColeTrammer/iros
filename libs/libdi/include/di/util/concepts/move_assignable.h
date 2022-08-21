#pragma once

#include <di/util/concepts/assignable_from.h>

namespace di::util::concepts {
template<typename T>
concept MoveAssignable = AssignableFrom<T, T&&>;
}
