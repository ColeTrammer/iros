#pragma once

#include <di/container/algorithm/contains.h>
#include <di/container/algorithm/count_if.h>
#include <di/container/algorithm/mismatch.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>

namespace di::container {
namespace detail {
    struct IsPermutationFunction {
        template<concepts::ForwardIterator It1, concepts::SentinelFor<It1> Sent1, concepts::ForwardIterator It2,
                 concepts::SentinelFor<It2> Sent2, typename Proj1 = function::Identity,
                 typename Proj2 = function::Identity,
                 concepts::IndirectEquivalenceRelation<meta::Projected<It1, Sent1>, meta::Projected<It2, Sent2>> Pred =
                     function::Equal>
        constexpr bool operator()(It1 first1, Sent1 sent1, It2 first2, Sent2 sent2, Pred pred = {}, Proj1 proj1 = {},
                                  Proj2 proj2 = {}) const {
            if (container::distance(first1, sent1) != container::distance(first2, sent2)) {
                return false;
            }
            return is_permutation_same_sized(util::move(first1), sent1, util::move(first2), sent2, util::ref(pred),
                                             util::ref(proj1), util::ref(proj2));
        }

        template<concepts::ForwardContainer Con1, concepts::ForwardContainer Con2, typename Proj1 = function::Identity,
                 typename Proj2 = function::Identity,
                 concepts::IndirectEquivalenceRelation<meta::Projected<meta::ContainerIterator<Con1>, Proj1>,
                                                       meta::Projected<meta::ContainerIterator<Con2>, Proj2>>
                     Pred = function::Equal>
        constexpr bool operator()(Con1&& container1, Con2&& container2, Pred pred = {}, Proj1 proj1 = {},
                                  Proj2 proj2 = {}) const {
            if (container::distance(container1) != container::distance(container2)) {
                return false;
            }
            return is_permutation_same_sized(container::begin(container1), container::end(container1),
                                             container::begin(container2), container::end(container2), util::ref(pred),
                                             util::ref(proj1), util::ref(proj2));
        }

    private:
        template<typename It1, typename Sent1, typename It2, typename Sent2, typename Proj1, typename Proj2,
                 typename Pred>
        constexpr static bool is_permutation_same_sized(It1 first1, Sent1 sent1, It2 first2, Sent2 sent2, Pred pred,
                                                        Proj1 proj1, Proj2 proj2) {
            // Skip any part where the 2 containers are identical.
            auto [left, right] = container::mismatch(util::move(first1), sent1, util::move(first2), sent2,
                                                     util::ref(pred), util::ref(proj1), util::ref(proj2));

            for (auto it = left; it != sent1; ++it) {
                auto compare_to_current = [&]<typename T>(T&& value) -> bool {
                    return function::invoke(pred, function::invoke(proj1, *it), util::forward<T>(value));
                };

                // This element has already been checked, so skip it this round.
                if (it != container::find_if(left, it, compare_to_current, proj1)) {
                    continue;
                }

                // Now ensure that the count of elements of *it in [left, sent1)
                // is the same as the count of elements of *it in [right, sent2].
                auto count_right = container::count_if(right, sent2, compare_to_current, util::ref(proj2));
                if (count_right == 0 ||
                    count_right != container::count_if(it, sent1, compare_to_current, util::ref(proj1))) {
                    return false;
                }
            }
            return true;
        }
    };
}

constexpr inline auto is_permutation = detail::IsPermutationFunction {};
}
