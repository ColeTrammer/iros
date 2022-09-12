#pragma once

#include <di/meta/type_identity.h>

namespace di::concepts {
template<typename T>
concept CanReference = requires { typename meta::TypeIdentity<T&>; };
}
