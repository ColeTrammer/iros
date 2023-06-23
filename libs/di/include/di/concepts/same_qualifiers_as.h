#pragma once

#include <di/meta/core.h>
#include <di/meta/like.h>

namespace di::concepts {
template<typename T, typename U>
concept SameQualifiersAs = SameAs<meta::Like<T, int>, meta::Like<U, int>>;
}
