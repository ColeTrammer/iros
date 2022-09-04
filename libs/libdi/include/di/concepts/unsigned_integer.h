#pragma once

#include <di/concepts/integer.h>
#include <di/concepts/signed_integer.h>

namespace di::concepts {
template<typename T>
concept UnsignedInteger = Integer<T> && !
SignedInteger<T>;
}
