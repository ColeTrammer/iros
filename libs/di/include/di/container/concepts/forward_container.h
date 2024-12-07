#pragma once

#include "di/container/concepts/forward_iterator.h"
#include "di/container/concepts/input_container.h"

namespace di::concepts {
template<typename T>
concept ForwardContainer = InputContainer<T> && ForwardIterator<meta::ContainerIterator<T>>;
}
