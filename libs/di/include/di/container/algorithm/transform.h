#pragma once

#include <di/container/algorithm/in_in_out_result.h>
#include <di/container/algorithm/in_out_result.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>

namespace di::container {
namespace detail {
    struct TransformFunction {
        template<concepts::InputIterator It, concepts::SentinelFor<It> Sent, concepts::WeaklyIncrementable Out,
                 concepts::CopyConstructible F, typename Proj = function::Identity>
        requires(concepts::IndirectlyWritable<Out, meta::IndirectResult<F&, meta::Projected<It, Proj>>>)
        constexpr InOutResult<It, Out> operator()(It first, Sent last, Out output, F op, Proj proj = {}) const {
            for (; first != last; ++first, ++output) {
                *output = function::invoke(op, function::invoke(proj, *first));
            }
            return { util::move(first), util::move(output) };
        }

        template<concepts::InputContainer Con, concepts::WeaklyIncrementable Out, concepts::CopyConstructible F,
                 typename Proj = function::Identity>
        requires(concepts::IndirectlyWritable<
                 Out, meta::IndirectResult<F&, meta::Projected<meta::ContainerIterator<Con>, Proj>>>)
        constexpr InOutResult<meta::BorrowedIterator<Con>, Out> operator()(Con&& container, Out output, F op,
                                                                           Proj proj = {}) const {
            return (*this)(container::begin(container), container::end(container), util::move(output), util::ref(op),
                           util::ref(proj));
        }

        template<concepts::InputIterator It1, concepts::SentinelFor<It1> Sent1, concepts::InputIterator It2,
                 concepts::SentinelFor<It2> Sent2, concepts::WeaklyIncrementable Out, concepts::CopyConstructible F,
                 typename Proj1 = function::Identity, typename Proj2 = function::Identity>
        requires(concepts::IndirectlyWritable<
                 Out, meta::IndirectResult<F&, meta::Projected<It1, Proj1>, meta::Projected<It2, Proj2>>>)
        constexpr InInOutResult<It1, It2, Out> operator()(It1 first1, Sent1 last1, It2 first2, Sent2 last2, Out output,
                                                          F op, Proj1 proj1 = {}, Proj2 proj2 = {}) const {
            for (; first1 != last1 && first2 != last2; ++first1, ++first2, ++output) {
                *output = function::invoke(op, function::invoke(proj1, *first1), function::invoke(proj2, *first2));
            }
            return { util::move(first1), util::move(first2), util::move(output) };
        }

        template<concepts::InputContainer Con1, concepts::InputContainer Con2, concepts::WeaklyIncrementable Out,
                 concepts::CopyConstructible F, typename Proj1 = function::Identity,
                 typename Proj2 = function::Identity>
        requires(concepts::IndirectlyWritable<
                 Out, meta::IndirectResult<F&, meta::Projected<meta::ContainerIterator<Con1>, Proj1>,
                                           meta::Projected<meta::ContainerIterator<Con2>, Proj2>>>)
        constexpr InInOutResult<meta::BorrowedIterator<Con1>, meta::BorrowedIterator<Con2>, Out>
        transform(Con1&& r1, Con2&& r2, Out output, F op, Proj1 proj1 = {}, Proj2 proj2 = {}) {
            return (*this)(container::begin(r1), container::end(r1), container::begin(r2), container::end(r2),
                           util::move(output), util::ref(op), util::ref(proj1), util::ref(proj2));
        }
    };
}

constexpr inline auto transform = detail::TransformFunction {};
}
