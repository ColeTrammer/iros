#pragma once

#include <di/concepts/rvalue_reference.h>
#include <di/meta/core.h>
#include <di/meta/remove_reference.h>

namespace di::meta {
template<typename T>
using RemoveRValueReference = Conditional<concepts::RValueReference<T>, RemoveReference<T>, T>;
}
