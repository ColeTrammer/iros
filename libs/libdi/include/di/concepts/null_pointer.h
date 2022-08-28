#pragma once

#include <di/concepts/same_as.h>
#include <di/meta/remove_cv.h>
#include <di/types/nullptr_t.h>

namespace di::concepts {
template<typename T>
concept NullPointer = SameAs<meta::RemoveCV<T>, types::nullptr_t>;
}
