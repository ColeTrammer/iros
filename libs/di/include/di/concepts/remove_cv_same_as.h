#pragma once

#include <di/meta/core.h>
#include <di/meta/remove_cv.h>

namespace di::concepts {
template<typename T, typename U>
concept RemoveCVSameAs = SameAs<meta::RemoveCV<T>, meta::RemoveCV<U>>;
}
