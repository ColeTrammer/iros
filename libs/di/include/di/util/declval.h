#pragma once

#include <di/concepts/always_false.h>
#include <di/meta/add_rvalue_reference.h>

namespace di::util {
template<typename T>
meta::AddRValueReference<T> declval() {
    static_assert(concepts::AlwaysFalse<T>, "declval() is only usable in non-evaluated contexts.");
}
}
