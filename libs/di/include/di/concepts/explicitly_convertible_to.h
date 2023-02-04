#pragma once

#include <di/util/declval.h>

namespace di::concepts {
template<typename From, typename To>
concept ExplicitlyConvertibleTo = requires { static_cast<To>(util::declval<From>()); };
}
