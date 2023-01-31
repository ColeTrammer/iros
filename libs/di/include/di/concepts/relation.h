#pragma once

#include <di/concepts/predicate.h>

namespace di::concepts {
template<typename R, typename T, typename U>
concept Relation = Predicate<R, T, T> && Predicate<R, U, U> && Predicate<R, T, U> && Predicate<R, U, T>;
}