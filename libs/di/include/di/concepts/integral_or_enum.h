#pragma once

#include <di/concepts/enum.h>
#include <di/concepts/integral.h>

namespace di::concepts {
template<typename T>
concept IntegralOrEnum = Integral<T> || Enum<T>;
}
