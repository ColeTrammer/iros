#pragma once

#include <di/util/concepts/lvalue_reference.h>
#include <di/util/concepts/rvalue_reference.h>

namespace di::util::concepts {
template<typename T>
concept Reference = LValueReference<T> || RValueReference<T>;
}
