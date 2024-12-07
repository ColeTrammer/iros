#pragma once

#include "di/container/concepts/bidirectional_iterator.h"
#include "di/container/concepts/uninit_forward_iterator.h"

namespace di::concepts {
template<typename T>
concept UninitBidirectionalIterator = UninitForwardIterator<T> && BidirectionalIterator<T>;
}
