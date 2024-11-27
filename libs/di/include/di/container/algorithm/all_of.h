#pragma once

#include <di/container/algorithm/find_if_not.h>
#include <di/function/as_bool.h>

namespace di::container {
namespace detail {
    struct AllOfFunction {
        template<concepts::InputIterator Iter, concepts::SentinelFor<Iter> Sent, typename Proj = function::Identity,
                 concepts::IndirectUnaryPredicate<meta::Projected<Iter, Proj>> Pred = function::AsBool>
        constexpr auto operator()(Iter first, Sent last, Pred pred = {}, Proj proj = {}) const -> bool {
            return container::find_if_not(util::move(first), last, util::ref(pred), util::ref(proj)) == last;
        }

        template<concepts::InputContainer Con, typename Proj = function::Identity,
                 concepts::IndirectUnaryPredicate<meta::Projected<meta::ContainerIterator<Con>, Proj>> Pred =
                     function::AsBool>
        constexpr auto operator()(Con&& container, Pred pred = {}, Proj proj = {}) const -> bool {
            return (*this)(container::begin(container), container::end(container), util::ref(pred), util::ref(proj));
        }
    };
}

constexpr inline auto all_of = detail::AllOfFunction {};
}

namespace di {
using container::all_of;
}
