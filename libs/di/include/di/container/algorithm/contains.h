#pragma once

#include <di/container/algorithm/find.h>

namespace di::container {
namespace detail {
    struct ContainsFunction {
        template<concepts::InputIterator It, concepts::SentinelFor<It> Sent, typename T,
                 typename Proj = function::Identity>
        requires(concepts::IndirectBinaryPredicate<function::Equal, meta::Projected<It, Proj>, T const*>)
        constexpr auto operator()(It first, Sent last, T const& needle, Proj proj = {}) const -> bool {
            return container::find(util::move(first), last, needle, util::ref(proj)) != last;
        }

        template<concepts::InputContainer Con, typename T, typename Proj = function::Identity>
        requires(concepts::IndirectBinaryPredicate<function::Equal, meta::Projected<meta::ContainerIterator<Con>, Proj>,
                                                   T const*>)
        constexpr auto operator()(Con&& container, T const& needle, Proj proj = {}) const -> bool {
            return (*this)(container::begin(container), container::end(container), needle, util::ref(proj));
        }
    };
}

constexpr inline auto contains = detail::ContainsFunction {};
}

namespace di {
using container::contains;
}
