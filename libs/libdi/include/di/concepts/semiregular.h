#pragma once

#include <di/concepts/copyable.h>
#include <di/concepts/default_initializable.h>

namespace di::concepts {
template<typename T>
concept Semiregular = Copyable<T> && DefaultInitializable<T>;
}
