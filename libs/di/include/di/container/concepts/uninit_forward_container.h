#pragma once

#include <di/container/concepts/uninit_forward_iterator.h>
#include <di/container/concepts/uninit_input_container.h>

namespace di::concepts {
template<typename T>
concept UninitForwardContainer = UninitInputContainer<T> && UninitForwardIterator<meta::ContainerIterator<T>>;
}
