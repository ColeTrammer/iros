#pragma once

#include <di/concepts/integral.h>
#include <di/concepts/signed_integral.h>

namespace di::concepts {
template<typename T>
concept UnsignedIntegral = Integral<T> && !
SignedIntegral<T>;
}
