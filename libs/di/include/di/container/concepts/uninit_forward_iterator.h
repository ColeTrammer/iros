#pragma once

#include "di/container/concepts/forward_iterator.h"
#include "di/container/concepts/uninit_input_iterator.h"
#include "di/container/concepts/uninit_sentinel_for.h"

namespace di::concepts {
template<typename T>
concept UninitForwardIterator = UninitInputIterator<T> && ForwardIterator<T> && UninitSentinelFor<T, T>;
}
