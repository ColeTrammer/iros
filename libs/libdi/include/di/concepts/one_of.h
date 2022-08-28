#pragma once

#include <di/concepts/same_as.h>

namespace di::concepts {
template<typename T, typename... Types>
concept OneOf = (SameAs<T, Types> || ...);
}
