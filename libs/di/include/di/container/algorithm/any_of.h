#pragma once

#include <di/container/algorithm/find_if.h>
#include <di/function/as_bool.h>

namespace di::container {
namespace detail {
    struct AnyOfFunction {
        template<concepts::InputIterator Iter, concepts::SentinelFor<Iter> Sent, typename Proj = function::Identity,
                 concepts::IndirectUnaryPredicate<meta::Projected<Iter, Proj>> Pred = function::AsBool>
        constexpr bool operator()(Iter first, Sent last, Pred pred = {}, Proj proj = {}) const {
            return container::find_if(util::move(first), last, util::ref(pred), util::ref(proj)) != last;
        }

        template<concepts::InputContainer Con, typename Proj = function::Identity,
                 concepts::IndirectUnaryPredicate<meta::Projected<meta::ContainerIterator<Con>, Proj>> Pred =
                     function::AsBool>
        constexpr bool operator()(Con&& container, Pred pred = {}, Proj proj = {}) const {
            return (*this)(container::begin(container), container::end(container), util::ref(pred), util::ref(proj));
        }
    };
}

constexpr inline auto any_of = detail::AnyOfFunction {};
}

namespace di {
using container::any_of;
}
