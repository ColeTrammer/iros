#pragma once

#include <di/container/algorithm/find.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>

namespace di::container {
namespace detail {
    struct RemoveFunction {
        template<concepts::Permutable It, concepts::SentinelFor<It> Sent, typename T,
                 typename Proj = function::Identity>
        requires(concepts::IndirectBinaryPredicate<function::Equal, meta::Projected<It, Proj>, T const*>)
        constexpr View<It> operator()(It first, Sent last, T const& value, Proj proj = {}) const {
            auto fast = container::find(util::move(first), last, value, util::ref(proj));
            if (fast == last) {
                return { fast, fast };
            }

            auto slow = fast++;
            for (; fast != last; ++fast) {
                if (value != function::invoke(proj, *fast)) {
                    *slow = container::iterator_move(fast);
                    ++slow;
                }
            }
            return { util::move(slow), util::move(fast) };
        }

        template<concepts::ForwardContainer Con, typename T, typename Proj = function::Identity>
        requires(concepts::Permutable<meta::ContainerIterator<Con>> &&
                 concepts::IndirectBinaryPredicate<function::Equal, meta::Projected<meta::ContainerIterator<Con>, Proj>,
                                                   T const*>)
        constexpr meta::BorrowedView<Con> operator()(Con&& container, T const& value, Proj proj = {}) const {
            return (*this)(container::begin(container), container::end(container), value, util::ref(proj));
        }
    };
}

constexpr inline auto remove = detail::RemoveFunction {};
}