#pragma once

#include <di/container/concepts/uninit_bidirectional_iterator.h>
#include <di/container/concepts/uninit_input_container.h>

namespace di::concepts {
template<typename T>
concept UninitBidirectionalContainer =
    UninitInputContainer<T> && UninitBidirectionalIterator<meta::ContainerIterator<T>>;
}
