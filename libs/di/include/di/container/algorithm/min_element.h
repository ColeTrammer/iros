#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/function/compare.h>
#include <di/function/identity.h>
#include <di/util/move.h>
#include <di/util/reference_wrapper.h>

namespace di::container {
namespace detail {
    struct MinElementFunction {
        template<concepts::ForwardIterator It, concepts::SentinelFor<It> Sent, typename Proj = function::Identity,
                 concepts::IndirectStrictWeakOrder<meta::Projected<It, Proj>> Comp = function::Compare>
        constexpr auto operator()(It first, Sent last, Comp comp = {}, Proj proj = {}) const -> It {
            if (first == last) {
                return first;
            }

            auto min_iter = first;
            for (auto it = ++first; it != last; ++it) {
                if (function::invoke(comp, function::invoke(proj, *it), function::invoke(proj, *min_iter)) < 0) {
                    min_iter = it;
                }
            }
            return min_iter;
        }

        template<concepts::ForwardContainer Con, typename Proj = function::Identity,
                 concepts::IndirectStrictWeakOrder<meta::Projected<meta::ContainerIterator<Con>, Proj>> Comp =
                     function::Compare>
        constexpr auto operator()(Con&& container, Comp comp = {}, Proj proj = {}) const
            -> meta::BorrowedIterator<Con> {
            return (*this)(container::begin(container), container::end(container), util::ref(comp), util::ref(proj));
        }
    };
}

constexpr inline auto min_element = detail::MinElementFunction {};
}

namespace di {
using container::min_element;
}
