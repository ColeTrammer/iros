#pragma once

#include <di/concepts/language_void.h>
#include <di/util/forward.h>

namespace di::concepts {
template<typename T, typename... Args>
concept ConstructibleFrom = (!LanguageVoid<T>) &&requires(Args&&... args) { T(util::forward<Args>(args)...); };
}
