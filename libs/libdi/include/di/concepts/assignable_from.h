#pragma once

#include <di/concepts/lvalue_reference.h>
#include <di/concepts/same_as.h>
#include <di/util/forward.h>

namespace di::concepts {
template<typename T, typename U>
concept AssignableFrom = LValueReference<T> && requires(T lvalue, U&& value) {
                                                   { lvalue = util::forward<U>(value) } -> SameAs<T>;
                                               };
}
