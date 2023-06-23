#pragma once

#include <di/meta/core.h>
#include <di/meta/decay.h>

namespace di::concepts {
template<typename T, typename U>
concept DecaySameAs = SameAs<meta::Decay<T>, meta::Decay<U>>;
}
