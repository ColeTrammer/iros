#pragma once

#include <di/container/algorithm/move.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/next.h>
#include <di/container/meta/prelude.h>
#include <di/container/view/view.h>
#include <di/util/move.h>

namespace di::container {
namespace detail {
    struct ShiftLeftFunction {
        template<concepts::Permutable It, concepts::SentinelFor<It> Sent>
        constexpr auto operator()(It first, Sent last, meta::IteratorSSizeType<It> n) const -> View<It> {
            DI_ASSERT(n >= 0);

            if (n == 0) {
                return { first, container::next(first, last) };
            }

            auto new_start = first;
            container::advance(new_start, n, last);

            if (new_start == last) {
                return { first, first };
            }

            auto result = container::move(util::move(new_start), last, first);
            return { first, result.out };
        }

        template<concepts::ForwardContainer Con>
        requires(concepts::Permutable<meta::ContainerIterator<Con>>)
        constexpr auto operator()(Con&& container, meta::ContainerSSizeType<Con> n) const -> meta::BorrowedView<Con> {
            return (*this)(container::begin(container), container::end(container), n);
        }
    };
}

constexpr inline auto shift_left = detail::ShiftLeftFunction {};
}

namespace di {
using container::shift_left;
}
