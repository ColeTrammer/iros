#pragma once

#include <di/container/algorithm/is_sorted_until.h>
#include <di/container/concepts/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/function/compare.h>
#include <di/function/identity.h>
#include <di/util/move.h>
#include <di/util/reference_wrapper.h>

namespace di::container {
namespace detail {
    struct IsHeapUntilFunction {
        template<concepts::RandomAccessIterator It, concepts::SentinelFor<It> Sent, typename Proj = function::Identity,
                 concepts::IndirectStrictWeakOrder<meta::Projected<It, Proj>> Comp = function::Compare>
        constexpr It operator()(It it, Sent last, Comp comp = {}, Proj proj = {}) const {
            auto n = container::distance(it, last);
            return is_heap_until_with_size(util::move(it), util::ref(comp), util::ref(proj), n);
        }

        template<concepts::RandomAccessContainer Con, typename Proj = function::Identity,
                 concepts::IndirectStrictWeakOrder<meta::Projected<meta::ContainerIterator<Con>, Proj>> Comp =
                     function::Compare>
        constexpr meta::BorrowedIterator<Con> operator()(Con&& container, Comp comp = {}, Proj proj = {}) const {
            return is_heap_until_with_size(container::begin(container), util::ref(comp), util::ref(proj),
                                           container::distance(container));
        }

    private:
        template<typename It, typename Comp, typename Proj, typename SSizeType>
        constexpr static It is_heap_until_with_size(It first, Comp comp, Proj proj, SSizeType n) {
            SSizeType parent = 0;
            for (SSizeType child = 1; child != n; ++child) {
                // If the parent is less than the child, the range is longer a max heap.
                if (function::invoke(comp, function::invoke(proj, first[parent]),
                                     function::invoke(proj, first[child])) < 0) {
                    return first + child;
                }

                // Increment the parent every other iteration.
                if (child % 2 == 0) {
                    ++parent;
                }
            }
            return first + n;
        }
    };
}

constexpr inline auto is_heap_until = detail::IsHeapUntilFunction {};
}

namespace di {
using container::is_heap_until;
}
