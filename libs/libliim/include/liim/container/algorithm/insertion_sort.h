#pragma once

#include <liim/container/concepts.h>
#include <liim/container/iterator/swap_iterator_contents.h>
#include <liim/container/producer/iterator_container.h>

namespace LIIM::Container::Algorithm {
struct InsertionSortFunction {
    template<MutableRandomAccessContainer C, ProjectionFor<ContainerValueType<C>> Proj = Identity,
             ThreeWayComparatorFor<Projected<Proj, ContainerValueType<C>>> Comp = CompareThreeWay>
    constexpr void operator()(C&& container, Comp compartor = {}, Proj projection = {}) const {
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
                if (invoke(compartor, invoke(projection, *--jt), invoke(projection, *kt)) <= 0) {
                    break;
                }
                swap_iterator_contents(jt, kt--);
            } while (jt != start);
        }
    }
};

constexpr inline auto insertion_sort = InsertionSortFunction {};
}
