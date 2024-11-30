#pragma once

#include <di/container/algorithm/move.h>
#include <di/container/algorithm/move_backward.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/next.h>
#include <di/container/meta/prelude.h>
#include <di/container/view/view.h>
#include <di/util/move.h>

namespace di::container {
namespace detail {
    struct ShiftRightFunction {
        template<concepts::Permutable It, concepts::SentinelFor<It> Sent>
        constexpr auto operator()(It first, Sent last, meta::IteratorSSizeType<It> n) const -> View<It> {
            DI_ASSERT(n >= 0);

            if (n == 0) {
                return { first, container::next(first, last) };
            }

            if constexpr (concepts::BidirectionalIterator<It>) {
                auto last_it = container::next(first, last);
                auto new_start = last_it;
                container::advance(new_start, -n, first);

                if (new_start == first) {
                    return { last_it, last_it };
                }

                // NOLINTNEXTLINE(readability-suspicious-call-argument)
                auto real_start = container::move_backward(first, new_start, last_it).out;
                return { real_start, last_it };
            } else {
                auto new_start = first;
                container::advance(new_start, n, last);
                if (new_start == last) {
                    return { new_start, new_start };
                }

                // Use fast and slow pointers, with the fast pointer
                // n elements ahead of the slow pointer. This approach
                // is outlined in the C++ paper for shift_left and shift_right.
                // See https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0769r0.pdf.
                auto fast = new_start;
                auto slow = first;

                // Fast path: the shift window is such that the size of the container is
                // less than twice the shift amount. This lets us do the shift directly,
                // without moving things around as we go.
                for (; slow != new_start; ++fast, ++slow) {
                    if (fast == last) {
                        auto result = container::move(first, slow, new_start);
                        return { new_start, result.out };
                    }
                }

                // Cyclic path, use the first n elements of the input as storage
                // for the elements which must be shifted into place. This approach essentially
                // works on blocks of size n. At each block, we swap the elements at the start
                // of the input with the elements of the current block. This makes the current
                // block point to the correct values, which retaining the pending elements which
                // otherwise would be overridden. Once we hit the end of the container, perform
                // a final move operation to finish the shift.
                for (;;) {
                    for (auto pending = first; pending != new_start; ++fast, ++slow, ++pending) {
                        if (fast == last) {
                            // Move the pending elements into place.
                            slow = container::move(pending, new_start, slow).out;

                            // Move the final elements into place.
                            auto last_it = container::move(first, pending, slow).out;
                            return { new_start, last_it };
                        }
                        // Swap the contents of slow and pending. The pending
                        // element of the previous iteration is now in the correct
                        // place, while pending now holds an element which will be
                        // shifted in later.
                        container::iterator_swap(pending, slow);
                    }
                }
            }
        }

        template<concepts::ForwardContainer Con>
        requires(concepts::Permutable<meta::ContainerIterator<Con>>)
        constexpr auto operator()(Con&& container, meta::ContainerSSizeType<Con> n) const -> meta::BorrowedView<Con> {
            return (*this)(container::begin(container), container::end(container), n);
        }
    };
}

constexpr inline auto shift_right = detail::ShiftRightFunction {};
}

namespace di {
using container::shift_right;
}
