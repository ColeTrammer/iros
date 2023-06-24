#pragma once

#include <di/container/algorithm/copy.h>
#include <di/container/algorithm/in_out_result.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>

namespace di::container {
namespace detail {
    struct SetDifferenceFunction {
        template<concepts::InputIterator It1, concepts::SentinelFor<It1> Sent1, concepts::InputIterator It2,
                 concepts::SentinelFor<It2> Sent2, concepts::WeaklyIncrementable Out, typename Comp = function::Compare,
                 typename Proj1 = function::Identity, typename Proj2 = function::Identity>
        requires(concepts::Mergeable<It1, It2, Out, Comp, Proj1, Proj2>)
        constexpr InOutResult<It1, Out> operator()(It1 first1, Sent1 last1, It2 first2, Sent2 last2, Out out,
                                                   Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const {
            // While both ranges are non-empty, see if the current element in range 1 is present in range 2.
            while (first1 != last1 && first2 != last2) {
                auto result =
                    function::invoke(comp, function::invoke(proj1, *first1), function::invoke(proj2, *first2));
                if (result == 0) {
                    // Element was equal, so skip.
                    ++first1;
                    ++first2;
                } else if (result < 0) {
                    // Element is before the current start of range 2. Therefore, it
                    // could not have been in range 2.
                    *out = *first1++;
                    ++out;
                } else {
                    // Element is after the current start of range 2. Advance range 2
                    // to see if the start of range 1 is present.
                    ++first2;
                }
            }

            // Copy and remaining parts of range 1 to output.
            return container::copy(util::move(first1), last1, util::move(out));
        }

        template<concepts::InputContainer Con1, concepts::InputContainer Con2, concepts::WeaklyIncrementable Out,
                 typename Comp = function::Compare, typename Proj1 = function::Identity,
                 typename Proj2 = function::Identity>
        requires(
            concepts::Mergeable<meta::ContainerIterator<Con1>, meta::ContainerIterator<Con2>, Out, Comp, Proj1, Proj2>)
        constexpr InOutResult<meta::BorrowedIterator<Con1>, Out> operator()(Con1&& container1, Con2&& container2,
                                                                            Out out, Comp comp = {}, Proj1 proj1 = {},
                                                                            Proj2 proj2 = {}) const {
            return (*this)(container::begin(container1), container::end(container1), container::begin(container2),
                           container::end(container2), util::move(out), util::ref(comp), util::ref(proj1),
                           util::ref(proj2));
        }
    };
}

constexpr inline auto set_difference = detail::SetDifferenceFunction {};
}

namespace di {
using container::set_difference;
}
