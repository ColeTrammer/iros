#pragma once

#include <di/util/concepts/bounded_language_array.h>
#include <di/util/concepts/unbounded_language_array.h>

namespace di::util::concepts {
template<typename T>
concept LanguageArray = BoundedLanguageArray<T> || UnboundedLanguageArray<T>;
}
