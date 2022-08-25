#pragma once

#include <di/util/concepts/constructible_from.h>

namespace di::util::concepts {
template<typename T>
concept DefaultConstructible = ConstructibleFrom<T>;
}
