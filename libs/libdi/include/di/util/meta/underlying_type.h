#pragma once

#include <di/util/concepts/enum.h>

namespace di::util::meta {
template<concepts::Enum T>
using UnderlyingType = __underlying_type(T);
}
