#pragma once

#include <di/util/concepts/move_assignable.h>
#include <di/util/concepts/move_constructible.h>

namespace di::util::concepts {
template<typename T>
concept Movable = MoveConstructible<T> && MoveAssignable<T>;
}
