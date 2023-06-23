#pragma once

#include <di/meta/core.h>
#include <di/meta/remove_cvref.h>

namespace di::concepts {
template<typename T, typename U>
concept RemoveCVRefSameAs = SameAs<meta::RemoveCVRef<T>, meta::RemoveCVRef<U>>;
}
