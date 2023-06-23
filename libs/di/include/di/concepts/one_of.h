#pragma once

#include <di/meta/core.h>

namespace di::concepts {
template<typename T, typename... Types>
concept OneOf = (SameAs<T, Types> || ...);
}
