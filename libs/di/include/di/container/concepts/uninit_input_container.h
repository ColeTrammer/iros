#pragma once

#include "di/container/concepts/input_container.h"
#include "di/container/concepts/uninit_input_iterator.h"
#include "di/container/concepts/uninit_sentinel_for.h"
#include "di/container/meta/container_iterator.h"
#include "di/container/meta/container_sentinel.h"

namespace di::concepts {
template<typename T>
concept UninitInputContainer = Container<T> && UninitInputIterator<meta::ContainerIterator<T>> &&
                               UninitSentinelFor<meta::ContainerSentinel<T>, meta::ContainerIterator<T>>;
}
