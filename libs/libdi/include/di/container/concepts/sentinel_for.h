#pragma once

#include <di/concepts/semiregular.h>
#include <di/concepts/weakly_equality_comparable_with.h>
#include <di/container/concepts/iterator.h>

namespace di::concepts {
template<typename Sent, typename Iter>
concept SentinelFor = Semiregular<Sent> && Iterator<Iter> && detail::WeaklyEqualityComparableWith<Sent, Iter>;
}
