#pragma once

#include "di/meta/core.h"
#include "di/meta/language.h"

namespace di::util {
template<typename T>
auto declval() -> meta::AddRValueReference<T> {
    static_assert(concepts::AlwaysFalse<T>, "declval() is only usable in non-evaluated contexts.");
}
}

namespace di {
using util::declval;
}
