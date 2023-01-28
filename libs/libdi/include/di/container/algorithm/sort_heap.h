#pragma once

#include <di/container/algorithm/pop_heap.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/distance.h>
#include <di/container/iterator/next.h>
#include <di/container/meta/prelude.h>
#include <di/function/compare.h>

namespace di::container {
namespace detail {
    struct SortHeapFunction {
        template<concepts::RandomAccessIterator It, concepts::SentinelFor<It> Sent, typename Comp = function::Compare,
                 typename Proj = function::Identity>
        requires(concepts::Sortable<It, Comp, Proj>)
        constexpr It operator()(It first, Sent last, Comp comp = {}, Proj proj = {}) const {
            if (first == last) {
                return first;
            }

            auto result = container::pop_heap(first, last, util::ref(comp), util::ref(proj));
            for (--last; first != last; --last) {
                container::pop_heap(first, last, util::ref(comp), util::ref(proj));
            }
            return result;
        }

        template<concepts::RandomAccessContainer Con, typename Comp = function::Compare,
                 typename Proj = function::Identity>
        requires(concepts::Sortable<meta::ContainerIterator<Con>, Comp, Proj>)
        constexpr meta::BorrowedIterator<Con> operator()(Con&& container, Comp comp = {}, Proj proj = {}) const {
            return (*this)(container::begin(container), container::end(container), util::ref(comp), util::ref(proj));
        }
    };
}

constexpr inline auto sort_heap = detail::SortHeapFunction {};
}