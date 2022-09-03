#pragma once

#include <di/concepts/const.h>
#include <di/concepts/lvalue_reference.h>
#include <di/meta/remove_reference.h>

namespace di::concepts {
template<typename T>
concept ConstLValueReference = LValueReference<T> && Const<meta::RemoveReference<T>>;
}
