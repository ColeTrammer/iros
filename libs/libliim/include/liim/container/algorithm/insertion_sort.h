#pragma once

#include <liim/container/concepts.h>
#include <liim/container/iterator/swap_iterator_contents.h>
#include <liim/container/producer/iterator_container.h>

namespace LIIM::Container::Algorithm {
template<MutableRandomAccessContainer C, ThreeWayComparatorFor<ContainerValueType<C>> Comp = CompareThreeWay>
constexpr void insertion_sort(C&& container, Comp compartor = {}) {
    auto start = container.begin();
    auto end = container.end();
    auto size = end - start;
    if (size <= 1) {
        return;
    }

    for (auto it = start + 1; it != end; ++it) {
        auto jt = it;
        auto kt = it;
        do {
            if (invoke(compartor, *--jt, *kt) <= 0) {
                break;
            }
            swap_iterator_contents(jt, kt--);
        } while (jt != start);
    }
}
}

using LIIM::Container::Algorithm::insertion_sort;
