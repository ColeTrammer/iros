#pragma once

#include <di/concepts/assignable_from.h>

namespace di::concepts {
template<typename T>
concept CopyAssignable = AssignableFrom<T&, T const&>;
}
