#pragma once

#include <di/concepts/trivially_constructible_from.h>

namespace di::concepts {
template<typename T>
concept TriviallyCopyConstructible = TriviallyConstructibleFrom<T, T const&>;
}
