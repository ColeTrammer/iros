#pragma once

#include <di/container/algorithm/in_out_result.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>

namespace di::container {
namespace detail {
    struct RemoveCopyIfFunction {
        template<concepts::InputIterator It, concepts::SentinelFor<It> Sent, concepts::WeaklyIncrementable Out,
                 typename Proj = function::Identity, concepts::IndirectUnaryPredicate<meta::Projected<It, Proj>> Pred>
        requires(concepts::IndirectlyCopyable<It, Out>)
        constexpr InOutResult<It, Out> operator()(It first, Sent last, Out output, Pred pred, Proj proj = {}) const {
            for (; first != last; ++first) {
                if (!function::invoke(pred, function::invoke(proj, *first))) {
                    *output = *first;
                    ++output;
                }
            }
            return { util::move(first), util::move(output) };
        }

        template<concepts::ForwardContainer Con, concepts::WeaklyIncrementable Out, typename Proj = function::Identity,
                 concepts::IndirectUnaryPredicate<meta::Projected<meta::ContainerIterator<Con>, Proj>> Pred>
        requires(concepts::IndirectlyCopyable<meta::ContainerIterator<Con>, Out>)
        constexpr InOutResult<meta::BorrowedIterator<Con>, Out> operator()(Con&& container, Out output, Pred pred,
                                                                           Proj proj = {}) const {
            return (*this)(container::begin(container), container::end(container), util::move(output), util::ref(pred),
                           util::ref(proj));
        }
    };
}

constexpr inline auto remove_copy_if = detail::RemoveCopyIfFunction {};
}
