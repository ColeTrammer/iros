#pragma once

#include <di/concepts/same_as.h>
#include <di/meta/decay.h>

namespace di::concepts {
template<typename T, typename U>
concept DecaySameAs = SameAs<meta::Decay<T>, meta::Decay<U>>;
}
