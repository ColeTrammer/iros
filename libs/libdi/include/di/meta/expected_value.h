#pragma once

#include <di/concepts/expected.h>
#include <di/meta/remove_cv.h>

namespace di::meta {
template<concepts::Expected T>
using ExpectedValue = meta::RemoveCV<T>::Value;
}
