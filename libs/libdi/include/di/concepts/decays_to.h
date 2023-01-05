#pragma once

#include <di/concepts/same_as.h>
#include <di/meta/decay.h>

namespace di::concepts {
template<typename From, typename To>
concept DecaysTo = SameAs<meta::Decay<From>, To>;
}