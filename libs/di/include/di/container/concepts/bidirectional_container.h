#pragma once

#include "di/container/concepts/bidirectional_iterator.h"
#include "di/container/concepts/forward_container.h"

namespace di::concepts {
template<typename T>
concept BidirectionalContainer = ForwardContainer<T> && BidirectionalIterator<meta::ContainerIterator<T>>;
}
