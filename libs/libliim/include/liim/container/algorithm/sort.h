#pragma once

#include <liim/container/algorithm/insertion_sort.h>
#include <liim/container/concepts.h>
#include <liim/container/iterator/swap_iterator_contents.h>
#include <liim/container/producer/iterator_container.h>

namespace LIIM::Container::Algorithm {
template<MutableRandomAccessContainer C, ThreeWayComparatorFor<ContainerValueType<C>> Comp = CompareThreeWay>
constexpr void sort(C&& container, Comp comparator = {}) {
    auto start = container.begin();
    auto end = container.end();

    // If the container is small, just use insertion sort.
    auto size = end - start;
    if (size <= 16) {
        return insertion_sort(iterator_container(start, end), comparator);
    }

    // Find the pivot element using the median of 3 rule, and place the median in start.
    auto pivot = start;
    auto mid = start + (end - start) / 2;
    auto last = end - 1;

    auto start_mid = invoke(comparator, *start, *mid);
    auto mid_last = invoke(comparator, *mid, *last);
    auto start_last = invoke(comparator, *start, *last);
    if ((start_mid < 0 && start_last > 0 && mid_last < 0) || (mid_last > 0 && start_last > 0 && start_mid > 0)) {
        swap_iterator_contents(start, mid);
    } else if ((start_mid < 0 && start_last < 0 && mid_last > 0) || (start_mid > 0 && mid_last > 0 && start_last < 0)) {
        swap_iterator_contents(start, last);
    }

    // Partition elements around the pivot.
    for (auto it = start + 1; it != end; ++it) {
        if (invoke(comparator, *it, *pivot) < 0) {
            swap_iterator_contents(it, pivot + 1);
            swap_iterator_contents(pivot, pivot + 1);
        }
    }

    // Recursively sort the range.
    sort(iterator_container(start, pivot), comparator);
    sort(iterator_container(pivot + 1, end), comparator);
}
}

using LIIM::Container::Algorithm::sort;
