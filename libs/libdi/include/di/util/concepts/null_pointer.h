#pragma once

#include <di/util/concepts/same_as.h>
#include <di/util/meta/remove_cv.h>
#include <di/util/types/nullptr_t.h>

namespace di::util::concepts {
template<typename T>
concept NullPointer = SameAs<meta::RemoveCV<T>, util::types::nullptr_t>;
}
