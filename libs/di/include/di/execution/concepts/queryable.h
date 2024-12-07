#pragma once

#include "di/meta/operations.h"

namespace di::concepts {
template<typename T>
concept Queryable = Destructible<T>;
}
