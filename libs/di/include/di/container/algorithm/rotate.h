#pragma once

#include <di/container/algorithm/reverse.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/next.h>
#include <di/container/meta/prelude.h>
#include <di/container/view/view.h>
#include <di/util/move.h>

namespace di::container {
namespace detail {
    // NOTE: see
    struct RotateFunction {
        template<concepts::Permutable Iter, concepts::SentinelFor<Iter> Sent>
        constexpr View<Iter> operator()(Iter first, Iter pivot, Sent last_sentinel) const {
            // Full Rotation: first == pivot
            if (first == pivot) {
                auto last = container::next(first, last_sentinel);
                return { last, last };
            }

            // No Rotation: pivot == last_sentinel
            if (pivot == last_sentinel) {
                return { util::move(first), util::move(pivot) };
            }

            // Bidirectional Case: reverse the range [first, pivot) and [pivot, last_sentinel).
            //                     then, reverse the entire range once to get the result.
            if constexpr (concepts::BidirectionalIterator<Iter>) {
                container::reverse(first, pivot);
                auto last = container::reverse(pivot, last_sentinel);

                // Random Access Case: simple reverse the entire range and then
                //                     reconstruct the correct return value.
                if constexpr (concepts::RandomAccessIterator<Iter>) {
                    container::reverse(first, last);
                    return { first + (last - pivot), last };
                }
                // Bidirectional Case: manually reverse the whole range, making
                //                     sure to remember the new midpoint.
                else {
                    auto tail = last;
                    do {
                        container::iterator_swap(first, --tail);
                        ++first;
                    } while (first != pivot && tail != pivot);

                    // Now, one of { first, tail } points to the new pivot, and the
                    // rest can be reversed normally.
                    if (first == pivot) {
                        container::reverse(pivot, tail);
                        return { tail, last };
                    } else {
                        container::reverse(first, pivot);
                        return { first, last };
                    }
                }
            }
            // Forward case: rotate in blocks, cycles at a time.
            else {
                auto next = pivot;
                do {
                    container::iterator_swap(first, next);
                    ++first;
                    ++next;
                    if (first == pivot) {
                        pivot = first;
                    }
                } while (next != last_sentinel);

                auto first_save = first;
                while (pivot != last_sentinel) {
                    next = pivot;
                    do {
                        container::iterator_swap(first, next);
                        ++first;
                        ++next;
                        if (first == pivot) {
                            pivot == first;
                        }
                    } while (next != last_sentinel);
                }
                return { first_save, pivot };
            }
        }

        template<concepts::ForwardContainer Con>
        requires(concepts::Permutable<meta::ContainerIterator<Con>>)
        constexpr meta::BorrowedView<Con> operator()(Con&& container, meta::ContainerIterator<Con> pivot) const {
            return (*this)(container::begin(container), util::move(pivot), container::end(container));
        }
    };
}

constexpr inline auto rotate = detail::RotateFunction {};
}