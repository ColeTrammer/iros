#pragma once

#include <liim/container/algorithm/insertion_sort.h>
#include <liim/container/concepts.h>
#include <liim/container/iterator/swap_iterator_contents.h>
#include <liim/container/producer/iterator_container.h>

namespace LIIM::Container::Algorithm {
struct SortFunction {
    template<MutableRandomAccessContainer C, ProjectionFor<ContainerValueType<C>> Proj = Identity,
             ThreeWayComparatorFor<Projected<Proj, ContainerValueType<C>>> Comp = CompareThreeWay>
    constexpr void operator()(C&& container, Comp comparator = {}, Proj projection = {}) const {
        auto start = container.begin();
        auto end = container.end();

        // If the container is small, just use insertion sort.
        auto size = end - start;
        if (size <= 16) {
            return insertion_sort(iterator_container(start, end), comparator, projection);
        }

        // Find the pivot element using the median of 3 rule, and place the median in start.
        auto pivot = start;
        auto mid = start + (end - start) / 2;
        auto last = end - 1;

        auto compare = [&](auto&& a, auto&& b) {
            return invoke(comparator, invoke(projection, a), invoke(projection, b));
        };

        auto start_mid = compare(*start, *mid);
        auto mid_last = compare(*mid, *last);
        auto start_last = compare(*start, *last);
        if ((start_mid < 0 && start_last > 0 && mid_last < 0) || (mid_last > 0 && start_last > 0 && start_mid > 0)) {
            swap_iterator_contents(start, mid);
        } else if ((start_mid < 0 && start_last < 0 && mid_last > 0) || (start_mid > 0 && mid_last > 0 && start_last < 0)) {
            swap_iterator_contents(start, last);
        }

        // Partition elements around the pivot.
        for (auto it = start + 1; it != end; ++it) {
            if (compare(*it, *pivot) < 0) {
                swap_iterator_contents(it, pivot + 1);
                swap_iterator_contents(pivot, pivot + 1);
            }
        }

        // Recursively sort the range.
        (*this)(iterator_container(start, pivot), comparator, projection);
        (*this)(iterator_container(pivot + 1, end), comparator, projection);
    }
};

constexpr inline auto sort = SortFunction {};
}
