#pragma once

#include <di/concepts/integral.h>
#include <di/concepts/signed.h>

namespace di::concepts {
template<typename T>
concept SignedIntegral = Integral<T> && Signed<T>;
}
