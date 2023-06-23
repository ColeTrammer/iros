#pragma once

#include <di/concepts/lvalue_reference.h>
#include <di/meta/core.h>
#include <di/util/forward.h>

namespace di::concepts {
template<typename T, typename U>
concept WeaklyAssignableFrom = requires(T lhs, U&& value) {
    { lhs = util::forward<U>(value) } -> SameAs<T>;
};
}
