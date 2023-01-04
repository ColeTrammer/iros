#pragma once

#include <di/meta/list/list_forward_declation.h>

namespace di::concepts {
template<typename T>
concept MetaInvocable = requires { typename meta::Quote<T::template Invoke>; };
}