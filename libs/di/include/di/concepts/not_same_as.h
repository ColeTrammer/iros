#pragma once

#include <di/meta/core.h>

namespace di::concepts {
template<typename T, typename U>
concept NotSameAs = (!SameAs<T, U>);
}
