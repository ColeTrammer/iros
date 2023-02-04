#pragma once

#include <di/container/concepts/indirect_strict_weak_order.h>
#include <di/container/concepts/indirectly_copyable.h>
#include <di/container/concepts/input_iterator.h>
#include <di/container/concepts/weakly_incrementable.h>
#include <di/container/meta/projected.h>
#include <di/function/compare.h>
#include <di/function/identity.h>

namespace di::concepts {
template<typename It1, typename It2, typename Out, typename Comp = function::Compare,
         typename Proj1 = function::Identity, typename Proj2 = function::Identity>
concept Mergeable =
    concepts::InputIterator<It1> && concepts::InputIterator<It2> && concepts::WeaklyIncrementable<Out> &&
    concepts::IndirectlyCopyable<It1, Out> && concepts::IndirectlyCopyable<It2, Out> &&
    concepts::IndirectStrictWeakOrder<Comp, meta::Projected<It1, Proj1>, meta::Projected<It2, Proj2>>;
}