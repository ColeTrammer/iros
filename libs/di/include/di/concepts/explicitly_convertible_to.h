#pragma once

#include <di/util/forward.h>

namespace di::concepts {
template<typename From, typename To>
concept ExplicitlyConvertibleTo = requires(From&& from) { static_cast<To>(util::forward<From>(from)); };
}
