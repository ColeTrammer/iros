#pragma once

#include "di/container/concepts/container.h"
#include "di/container/concepts/input_iterator.h"

namespace di::concepts {
template<typename T>
concept InputContainer = Container<T> && InputIterator<meta::ContainerIterator<T>>;
}
