#pragma once

#include <liim/container/concepts.h>

namespace LIIM::Container::Algorithm {
struct ContainsFunction {
    template<Container C, typename T = ContainerValueType<C>, ProjectionFor<ContainerValueType<C>> Proj = Identity,
             ComparatorFor<Projected<Proj, ContainerValueType<C>>, T> Comp = Equal>
    constexpr bool operator()(C&& container, const T& needle, Comp comparator = {}, Proj projection = {}) const {
        auto end = container.end();
        for (auto it = container.begin(); it != end; ++it) {
            if (invoke(comparator, invoke(projection, *it), needle)) {
                return true;
            }
        }
        return false;
    }
};

constexpr inline auto contains = ContainsFunction {};
}
