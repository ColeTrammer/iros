#pragma once

#include "di/container/algorithm/copy.h"
#include "di/container/concepts/prelude.h"
#include "di/container/iterator/prelude.h"
#include "di/container/meta/prelude.h"

namespace di::container {
namespace detail {
    struct IncludesFunction {
        template<concepts::InputIterator It1, concepts::SentinelFor<It1> Sent1, concepts::InputIterator It2,
                 concepts::SentinelFor<It2> Sent2, typename Proj1 = function::Identity,
                 typename Proj2 = function::Identity,
                 concepts::IndirectStrictWeakOrder<meta::Projected<It1, Proj1>, meta::Projected<It2, Proj2>> Comp =
                     function::Compare>
        constexpr auto operator()(It1 first1, Sent1 last1, It2 first2, Sent2 last2, Comp comp = {}, Proj1 proj1 = {},
                                  Proj2 proj2 = {}) const -> bool {
            // While both ranges are non-empty, see if every element of range 2 is in range 1.
            while (first1 != last1 && first2 != last2) {
                auto result =
                    function::invoke(comp, function::invoke(proj1, *first1), function::invoke(proj2, *first2));
                if (result > 0) {
                    return false;
                }
                if (result == 0) {
                    ++first1;
                    ++first2;
                } else {
                    ++first1;
                }
            }
            return first2 == last2;
        }

        template<concepts::InputContainer Con1, concepts::InputContainer Con2, typename Proj1 = function::Identity,
                 typename Proj2 = function::Identity,
                 concepts::IndirectStrictWeakOrder<meta::Projected<meta::ContainerIterator<Con1>, Proj1>,
                                                   meta::Projected<meta::ContainerIterator<Con2>, Proj2>>
                     Comp = function::Compare>
        constexpr auto operator()(Con1&& container1, Con2&& container2, Comp comp = {}, Proj1 proj1 = {},
                                  Proj2 proj2 = {}) const -> bool {
            return (*this)(container::begin(container1), container::end(container1), container::begin(container2),
                           container::end(container2), util::ref(comp), util::ref(proj1), util::ref(proj2));
        }
    };
}

constexpr inline auto includes = detail::IncludesFunction {};
}
