#pragma once

#include <di/util/forward.h>

namespace di::concepts {
template<typename T, typename... Args>
concept ConstructibleFrom = requires(Args&&... args) { T(util::forward<Args>(args)...); };
}
