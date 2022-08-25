#pragma once

#include <di/util/concepts/assignable_from.h>

namespace di::util::concepts {
template<typename T>
concept CopyAssignable = AssignableFrom<T&, T const&>;
}
