#pragma once

#include <di/concepts/class.h>
#include <di/concepts/decays_to.h>

namespace di::concepts {
template<typename T>
concept ClassType = DecaysTo<T, T> && Class<T>;
}
