#pragma once

#include <di/container/algorithm/min_max_result.h>
#include <di/container/algorithm/minmax_element.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>

namespace di::container {
namespace detail {
    struct MinMaxElementFunction {
        template<concepts::ForwardIterator It, concepts::SentinelFor<It> Sent, typename Proj = function::Identity,
                 concepts::IndirectStrictWeakOrder<meta::Projected<It, Proj>> Comp = function::Compare>
        constexpr MinMaxResult<It> operator()(It first, Sent last, Comp comp = {}, Proj proj = {}) const {
            if (first == last) {
                return { first, first };
            }

            auto min_iter = first;
            auto max_iter = first;
            for (auto it = ++first; it != last; ++it) {
                // Perform fancy algorithm to only do 3*N/2 - 2 comparisons, instead of 2*N comparisons.
                // The idea is to consider pairs of elements in a row, and first compare them. Based on that
                // result, we know whether to compare each element with the min or max element.

                auto jt = container::next(it);
                // Base case: only 1 element remains.
                if (jt == last) {
                    auto compare_with_max =
                        function::invoke(comp, function::invoke(proj, *it), function::invoke(proj, *max_iter));
                    if (compare_with_max >= 0) {
                        max_iter = it;
                    } else if (function::invoke(comp, function::invoke(proj, *it), function::invoke(proj, *min_iter)) <
                               0) {
                        min_iter = it;
                    }
                } else {
                    auto it_jt_result =
                        function::invoke(comp, function::invoke(proj, *it), function::invoke(proj, *jt));
                    if (it_jt_result < 0) {
                        // Compare it to min, jt to max.
                        if (function::invoke(comp, function::invoke(proj, *it), function::invoke(proj, *min_iter)) <
                            0) {
                            min_iter = it;
                        }
                        if (function::invoke(comp, function::invoke(proj, *jt), function::invoke(proj, *max_iter)) >=
                            0) {
                            max_iter = jt;
                        }
                    } else {
                        // Compare it to max, jt to min.
                        if (function::invoke(comp, function::invoke(proj, *it), function::invoke(proj, *min_iter)) >=
                            0) {
                            max_iter = it;
                        }
                        if (function::invoke(comp, function::invoke(proj, *jt), function::invoke(proj, *max_iter)) <
                            0) {
                            min_iter = jt;
                        }
                    }
                    ++it;
                }
            }
            return { util::move(min_iter), util::move(max_iter) };
        }

        template<concepts::ForwardContainer Con, typename Proj = function::Identity,
                 concepts::IndirectStrictWeakOrder<meta::Projected<meta::ContainerIterator<Con>, Proj>> Comp =
                     function::Compare>
        constexpr MinMaxResult<meta::BorrowedIterator<Con>> operator()(Con&& container, Comp comp = {},
                                                                       Proj proj = {}) const {
            return (*this)(container::begin(container), container::end(container), util::ref(comp), util::ref(proj));
        }
    };
}

constexpr inline auto minmax_element = detail::MinMaxElementFunction {};
}
