#pragma once

#include <di/concepts/expected_of.h>
#include <di/meta/core.h>

namespace di::concepts {
template<typename Fal, typename T>
concept MaybeFallible = SameAs<Fal, T> || ExpectedOf<Fal, T>;
}
