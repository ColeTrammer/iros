#pragma once

#include <di/concepts/enum.h>

namespace di::meta {
template<concepts::Enum T>
using UnderlyingType = __underlying_type(T);
}
