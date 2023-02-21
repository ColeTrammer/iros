#pragma once

#include <di/container/algorithm/in_out_result.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>

namespace di::container {
namespace detail {
    struct ReplaceCopyFunction {
        template<concepts::InputIterator It, concepts::SentinelFor<It> Sent, typename T, typename U,
                 concepts::OutputIterator<U const&> Out, typename Proj = function::Identity>
        requires(concepts::IndirectlyCopyable<It, Out> &&
                 concepts::IndirectBinaryPredicate<function::Equal, meta::Projected<It, Proj>, T const*>)
        constexpr InOutResult<It, Out> operator()(It first, Sent last, Out output, T const& old_value,
                                                  U const& new_value, Proj proj = {}) const {
            for (; first != last; ++first, ++output) {
                if (old_value == function::invoke(proj, *first)) {
                    *output = new_value;
                } else {
                    *output = *first;
                }
            }
            return { util::move(first), util::move(output) };
        }

        template<concepts::ForwardContainer Con, typename T, typename U, concepts::OutputIterator<U const&> Out,
                 typename Proj = function::Identity>
        requires(concepts::IndirectlyCopyable<meta::ContainerIterator<Con>, Out> &&
                 concepts::IndirectBinaryPredicate<function::Equal, meta::Projected<meta::ContainerIterator<Con>, Proj>,
                                                   T const*>)
        constexpr InOutResult<meta::BorrowedIterator<Con>, Out>
        operator()(Con&& container, Out output, T const& old_value, U const& new_value, Proj proj = {}) const {
            return (*this)(container::begin(container), container::end(container), util::move(output), old_value,
                           new_value, util::ref(proj));
        }
    };
}

constexpr inline auto replace_copy = detail::ReplaceCopyFunction {};
}
