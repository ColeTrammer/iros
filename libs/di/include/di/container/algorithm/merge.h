#pragma once

#include <di/container/algorithm/copy.h>
#include <di/container/algorithm/in_in_out_result.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>

namespace di::container {
namespace detail {
    struct MergeFunction {
        template<concepts::InputIterator It1, concepts::SentinelFor<It1> Sent1, concepts::InputIterator It2,
                 concepts::SentinelFor<It2> Sent2, concepts::WeaklyIncrementable Out, typename Comp = function::Compare,
                 typename Proj1 = function::Identity, typename Proj2 = function::Identity>
        requires(concepts::Mergeable<It1, It2, Out, Comp, Proj1, Proj2>)
        constexpr InInOutResult<It1, It2, Out> operator()(It1 first1, Sent1 last1, It2 first2, Sent2 last2, Out out,
                                                          Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const {
            // While both ranges are non-empty, compare them to find which element goes first.
            for (; first1 != last1 && first2 != last2; ++out) {
                if (function::invoke(comp, function::invoke(proj1, *first1), function::invoke(proj2, *first2)) < 0) {
                    *out = *first1++;
                } else {
                    *out = *first2++;
                }
            }

            // Copy and remaining parts of the input to output.
            auto [end1, out_next] = container::copy(util::move(first1), last1, util::move(out));
            auto [end2, out_final] = container::copy(util::move(first2), last2, util::move(out_next));
            return { util::move(end1), util::move(end2), util::move(out_final) };
        }

        template<concepts::InputContainer Con1, concepts::InputContainer Con2, concepts::WeaklyIncrementable Out,
                 typename Comp = function::Compare, typename Proj1 = function::Identity,
                 typename Proj2 = function::Identity>
        requires(
            concepts::Mergeable<meta::ContainerIterator<Con1>, meta::ContainerIterator<Con2>, Out, Comp, Proj1, Proj2>)
        constexpr InInOutResult<meta::BorrowedIterator<Con1>, meta::BorrowedIterator<Con2>, Out>
        operator()(Con1&& container1, Con2&& container2, Out out, Comp comp = {}, Proj1 proj1 = {},
                   Proj2 proj2 = {}) const {
            return (*this)(container::begin(container1), container::end(container1), container::begin(container2),
                           container::end(container2), util::move(out), util::ref(comp), util::ref(proj1),
                           util::ref(proj2));
        }
    };
}

constexpr inline auto merge = detail::MergeFunction {};
}

namespace di {
using container::merge;
}
