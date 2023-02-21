#pragma once

#include <di/concepts/same_as.h>

namespace di::concepts {
template<typename T, typename U>
concept NotSameAs = (!SameAs<T, U>);
}
