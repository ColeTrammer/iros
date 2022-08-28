#pragma once

#include <di/concepts/lvalue_reference.h>
#include <di/concepts/rvalue_reference.h>

namespace di::concepts {
template<typename T>
concept Reference = LValueReference<T> || RValueReference<T>;
}
