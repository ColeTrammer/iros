#pragma once

#include <di/util/forward.h>

namespace di::util::concepts {
template<typename From, typename To>
concept ExplicitlyConvertibleTo = requires(From&& from) { static_cast<To>(util::forward<From>(from)); };
}
