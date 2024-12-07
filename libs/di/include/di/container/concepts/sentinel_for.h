#pragma once

#include "di/container/concepts/iterator.h"
#include "di/meta/compare.h"
#include "di/meta/operations.h"

namespace di::concepts {
template<typename Sent, typename Iter>
concept SentinelFor = Semiregular<Sent> && Iterator<Iter> && detail::WeaklyEqualityComparableWith<Sent, Iter>;
}
