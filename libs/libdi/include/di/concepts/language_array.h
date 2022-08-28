#pragma once

#include <di/concepts/bounded_language_array.h>
#include <di/concepts/unbounded_language_array.h>

namespace di::concepts {
template<typename T>
concept LanguageArray = BoundedLanguageArray<T> || UnboundedLanguageArray<T>;
}
