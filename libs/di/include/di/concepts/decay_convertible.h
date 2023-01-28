#pragma once

#include <di/concepts/convertible_to.h>
#include <di/meta/decay.h>

namespace di::concepts {
template<typename T>
concept DecayConvertible = ConvertibleTo<T, meta::Decay<T>>;
}