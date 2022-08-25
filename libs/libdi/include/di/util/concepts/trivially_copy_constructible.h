#pragma once

#include <di/util/concepts/trivially_constructible_from.h>

namespace di::util::concepts {
template<typename T>
concept TriviallyCopyConstructible = TriviallyConstructibleFrom<T, T const&>;
}
