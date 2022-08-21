#pragma once

#include <di/util/concepts/enumeration.h>

namespace di::util::meta {
template<concepts::Enumeration T>
using UnderlyingType = __underlying_type(T);
}
