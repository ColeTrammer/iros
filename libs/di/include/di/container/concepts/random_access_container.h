#pragma once

#include <di/container/concepts/bidirectional_container.h>
#include <di/container/concepts/random_access_iterator.h>

namespace di::concepts {
template<typename T>
concept RandomAccessContainer = BidirectionalContainer<T> && RandomAccessIterator<meta::ContainerIterator<T>>;
}
