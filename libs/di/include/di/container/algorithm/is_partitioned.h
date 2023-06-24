#pragma once

#include <di/container/algorithm/find_if.h>
#include <di/container/algorithm/find_if_not.h>

namespace di::container {
namespace detail {
    struct IsPartitionedFunction {
        template<concepts::InputIterator It, concepts::SentinelFor<It> Sent, typename Proj = function::Identity,
                 concepts::IndirectUnaryPredicate<meta::Projected<It, Proj>> Pred>
        constexpr bool operator()(It first, Sent last, Pred pred, Proj proj = {}) const {
            auto mid = container::find_if_not(util::move(first), last, util::ref(pred), util::ref(proj));
            return container::find_if(util::move(mid), last, util::ref(pred), util::ref(proj)) == last;
        }

        template<concepts::InputContainer Con, typename Proj = function::Identity,
                 concepts::IndirectUnaryPredicate<meta::Projected<meta::ContainerIterator<Con>, Proj>> Pred>
        constexpr bool operator()(Con&& container, Pred pred, Proj proj = {}) const {
            return (*this)(container::begin(container), container::end(container), util::ref(pred), util::ref(proj));
        }
    };
}

constexpr inline auto is_partitioned = detail::IsPartitionedFunction {};
}

namespace di {
using container::is_partitioned;
}
