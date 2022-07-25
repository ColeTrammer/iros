#pragma once

#include <liim/container/concepts.h>
#include <liim/option.h>

namespace LIIM::Container::Algorithm {
struct IsSortedFunction {
    template<Container C, ProjectionFor<ContainerValueType<C>> Proj = Identity,
             ThreeWayComparatorFor<Projected<Proj, ContainerValueType<C>>> Comp = CompareThreeWay>
    constexpr bool operator()(C&& container, Comp comparator = {}, Proj projection = {}) const {
        auto start = container.begin();
        auto end = container.end();
        if (start == end) {
            return true;
        }

        // NOTE: use Option<> so that if the container type is a
        //       reference, we can safely rebind it.
        using Storage = Option<ContainerValueType<C>>;
        auto prev = Storage(*start);
        while (++start != end) {
            auto current = Storage(*start);
            if (invoke(comparator, invoke(projection, *prev), invoke(projection, *current)) > 0) {
                return false;
            }
            prev = move(current);
        }
        return true;
    }
};

constexpr inline auto is_sorted = IsSortedFunction {};
}
