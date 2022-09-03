#pragma once

#include <di/concepts/const.h>
#include <di/concepts/rvalue_reference.h>
#include <di/meta/remove_reference.h>

namespace di::concepts {
template<typename T>
concept MutableRValueReference = RValueReference<T> && (!Const<meta::RemoveReference<T>>);
}
