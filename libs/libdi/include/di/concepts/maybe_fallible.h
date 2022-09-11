#pragma once

#include <di/concepts/expected_of.h>
#include <di/concepts/same_as.h>

namespace di::concepts {
template<typename Fal, typename T>
concept MaybeFallible = SameAs<Fal, T> || ExpectedOf<Fal, T>;
}
