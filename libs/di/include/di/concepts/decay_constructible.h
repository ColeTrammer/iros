#pragma once

#include <di/concepts/constructible_from.h>
#include <di/meta/decay.h>

namespace di::concepts {
template<typename T>
concept DecayConstructible = ConstructibleFrom<meta::Decay<T>, T>;
}