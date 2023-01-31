#pragma once

#include <di/container/algorithm/adjacent_find.h>
#include <di/container/algorithm/in_out_result.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>

namespace di::container {
namespace detail {
    struct UniqueCopyFunction {
        template<concepts::InputIterator It, concepts::SentinelFor<It> Sent, concepts::WeaklyIncrementable Out,
                 typename Proj = function::Identity,
                 concepts::IndirectEquivalenceRelation<meta::Projected<It, Proj>> Comp = function::Equal>
        requires(concepts::IndirectlyCopyable<It, Out> &&
                 (concepts::ForwardIterator<It> ||
                  (concepts::InputIterator<Out> &&
                   concepts::SameAs<meta::IteratorValue<It>, meta::IteratorValue<Out>>) ||
                  concepts::IndirectlyCopyableStorable<It, Out>) )
        constexpr InOutResult<It, Out> operator()(It first, Sent last, Out out, Comp comp = {}, Proj proj = {}) const {
            if (first == last) {
                return { util::move(first), util::move(out) };
            }

            if constexpr (concepts::InputIterator<Out> &&
                          concepts::SameAs<meta::IteratorValue<It>, meta::IteratorValue<Out>>) {
                // Since out is an InputIterator, we can read the value's written to Out to check for equality.

                *out = *first;
                for (++first; first != last; ++first) {
                    if (!function::invoke(comp, function::invoke(proj, *out), function::invoke(proj, *first))) {
                        *out = *first;
                        ++out;
                    }
                }
                ++out;
            } else if constexpr (concepts::ForwardIterator<It>) {
                // Since first is an input iterator, and we are performing a copy, we can
                // read the previous value from first.

                *out = *first;
                ++out;
                auto prev = first;
                for (++first; first != last; ++first) {
                    if (!function::invoke(comp, function::invoke(proj, *prev), function::invoke(proj, *first))) {
                        prev = first;
                        *out = *first;
                        ++out;
                    }
                }
            } else {
                // Store the previous value directly, on the stack.

                meta::IteratorValue<It> prev = *first;
                *out = prev;
                ++out;
                for (++first; first != last; ++first) {
                    if (!function::invoke(comp, function::invoke(proj, prev), function::invoke(proj, *first))) {
                        prev = *first;
                        *out = prev;
                        ++out;
                    }
                }
            }

            return { util::move(first), util::move(out) };
        }

        template<concepts::InputContainer Con, concepts::WeaklyIncrementable Out, typename Proj = function::Identity,
                 concepts::IndirectEquivalenceRelation<meta::Projected<meta::ContainerIterator<Con>, Proj>> Comp =
                     function::Equal>
        requires(concepts::IndirectlyCopyable<meta::ContainerIterator<Con>, Out> &&
                 (concepts::ForwardIterator<meta::ContainerIterator<Con>> ||
                  (concepts::InputIterator<Out> &&
                   concepts::SameAs<meta::ContainerValue<Con>, meta::ContainerValue<Out>>) ||
                  concepts::IndirectlyCopyableStorable<meta::ContainerIterator<Con>, Out>) )
        constexpr InOutResult<meta::BorrowedIterator<Con>, Out> operator()(Con&& container, Out out, Comp comp = {},
                                                                           Proj proj = {}) const {
            return (*this)(container::begin(container), container::end(container), util::move(out), util::ref(comp),
                           util::ref(proj));
        }
    };
}

constexpr inline auto unique_copy = detail::UniqueCopyFunction {};
}