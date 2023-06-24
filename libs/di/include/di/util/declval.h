#pragma once

#include <di/meta/core.h>
#include <di/meta/language.h>

namespace di::util {
template<typename T>
meta::AddRValueReference<T> declval() {
    static_assert(concepts::AlwaysFalse<T>, "declval() is only usable in non-evaluated contexts.");
}
}
