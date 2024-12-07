#pragma once

#include "di/container/algorithm/make_heap.h"
#include "di/container/algorithm/sort_heap.h"

namespace di::container {
namespace detail {
    struct SortFunction {
        template<concepts::RandomAccessIterator It, concepts::SentinelFor<It> Sent, typename Comp = function::Compare,
                 typename Proj = function::Identity>
        requires(concepts::Sortable<It, Comp, Proj>)
        constexpr auto operator()(It first, Sent last, Comp comp = {}, Proj proj = {}) const -> It {
            // FIXME: consider using quick sort instead of heap sort.
            container::make_heap(first, last, util::ref(comp), util::ref(proj));
            return container::sort_heap(first, last, util::ref(comp), util::ref(proj));
        }

        template<concepts::RandomAccessContainer Con, typename Comp = function::Compare,
                 typename Proj = function::Identity>
        requires(concepts::Sortable<meta::ContainerIterator<Con>, Comp, Proj>)
        constexpr auto operator()(Con&& container, Comp comp = {}, Proj proj = {}) const
            -> meta::BorrowedIterator<Con> {
            return (*this)(container::begin(container), container::end(container), util::ref(comp), util::ref(proj));
        }
    };
}

constexpr inline auto sort = detail::SortFunction {};
}

namespace di {
using container::sort;
}
