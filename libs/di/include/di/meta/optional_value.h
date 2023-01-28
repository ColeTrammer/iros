#pragma once

#include <di/concepts/optional.h>
#include <di/meta/remove_cvref.h>

namespace di::meta {
template<concepts::Optional T>
using OptionalValue = meta::RemoveCVRef<T>::Value;
}
