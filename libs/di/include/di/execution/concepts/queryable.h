#pragma once

#include <di/concepts/destructible.h>

namespace di::concepts {
template<typename T>
concept Queryable = Destructible<T>;
}
