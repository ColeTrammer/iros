#pragma once

#include <di/concepts/integer.h>
#include <di/concepts/signed.h>

namespace di::concepts {
template<typename T>
concept SignedInteger = Integer<T> && Signed<T>;
}
