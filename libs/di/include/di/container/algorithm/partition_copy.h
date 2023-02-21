#pragma once

#include <di/container/algorithm/in_out_out_result.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>

namespace di::container {
namespace detail {
    struct PartitionCopyFunction {
        template<concepts::InputIterator It, concepts::SentinelFor<It> Sent, concepts::WeaklyIncrementable OutTrue,
                 concepts::WeaklyIncrementable OutFalse, typename Proj = function::Identity,
                 concepts::IndirectUnaryPredicate<meta::Projected<It, Proj>> Pred>
        requires(concepts::IndirectlyCopyable<It, OutTrue> && concepts::IndirectlyCopyable<It, OutFalse>)
        constexpr InOutOutResult<It, OutTrue, OutFalse>
        operator()(It first, Sent last, OutTrue out_true, OutFalse out_false, Pred pred, Proj proj = {}) const {
            for (; first != last; ++first) {
                if (function::invoke(pred, function::invoke(proj, *first))) {
                    *out_true = *first;
                    ++out_true;
                } else {
                    *out_false = *first;
                    ++out_false;
                }
            }
            return { util::move(first), util::move(out_true), util::move(out_false) };
        }

        template<concepts::InputContainer Con, concepts::WeaklyIncrementable OutTrue,
                 concepts::WeaklyIncrementable OutFalse, typename Proj = function::Identity,
                 concepts::IndirectUnaryPredicate<meta::Projected<meta::ContainerIterator<Con>, Proj>> Pred>
        requires(concepts::IndirectlyCopyable<meta::ContainerIterator<Con>, OutTrue> &&
                 concepts::IndirectlyCopyable<meta::ContainerIterator<Con>, OutFalse>)
        constexpr InOutOutResult<meta::BorrowedIterator<Con>, OutTrue, OutFalse>
        operator()(Con&& container, OutTrue out_true, OutFalse out_false, Pred pred, Proj proj = {}) const {
            return (*this)(container::begin(container), container::end(container), util::move(out_true),
                           util::move(out_false), util::ref(pred), util::ref(proj));
        }
    };
}

constexpr inline auto partition_copy = detail::PartitionCopyFunction {};
}
