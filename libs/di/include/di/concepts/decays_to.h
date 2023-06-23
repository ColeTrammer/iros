#pragma once

#include <di/meta/core.h>
#include <di/meta/decay.h>

namespace di::concepts {
template<typename From, typename To>
concept DecaysTo = SameAs<meta::Decay<From>, To>;
}
