#pragma once

#include <di/concepts/move_assignable.h>
#include <di/concepts/move_constructible.h>

namespace di::concepts {
template<typename T>
concept Movable = MoveConstructible<T> && MoveAssignable<T>;
}
