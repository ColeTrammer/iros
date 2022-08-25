#pragma once

#include <di/util/meta/decay.h>
#include <di/util/meta/unwrap_reference.h>

namespace di::util::meta {
template<typename T>
using UnwrapRefDecay = UnwrapRererence<Decay<T>>;
}
