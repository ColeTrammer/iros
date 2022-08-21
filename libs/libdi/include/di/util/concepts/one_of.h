#pragma once

#include <di/util/concepts/same_as.h>

namespace di::util::concepts {
template<typename T, typename... Types>
concept OneOf = (SameAs<T, Types> || ...);
}
