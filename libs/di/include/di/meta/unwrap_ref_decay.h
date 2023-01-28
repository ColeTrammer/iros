#pragma once

#include <di/meta/decay.h>
#include <di/meta/unwrap_reference.h>

namespace di::meta {
template<typename T>
using UnwrapRefDecay = UnwrapReference<Decay<T>>;
}
