#pragma once

#include <di/concepts/assignable_from.h>

namespace di::concepts {
template<typename T>
concept MoveAssignable = AssignableFrom<T&, T>;
}
