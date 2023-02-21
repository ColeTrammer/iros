#pragma once

#include <di/concepts/decay_constructible.h>
#include <di/concepts/move_constructible.h>

namespace di::concepts {
template<typename T>
concept MovableValue = DecayConstructible<T> && MoveConstructible<meta::Decay<T>>;
}
