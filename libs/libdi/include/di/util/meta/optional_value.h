#pragma once

#include <di/util/concepts/optional.h>
#include <di/util/meta/remove_cv.h>

namespace di::util::meta {
template<concepts::Optional T>
using OptionalValue = meta::RemoveCV<T>::Value;
}
