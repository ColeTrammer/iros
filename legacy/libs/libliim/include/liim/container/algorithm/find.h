#pragma once

#include <liim/container/concepts.h>
#include <liim/option.h>

namespace LIIM::Container::Algorithm {
struct FindFunction {
    template<Container C, typename T = ContainerValueType<C>, ProjectionFor<ContainerValueType<C>> Proj = Identity,
             ComparatorFor<Projected<Proj, ContainerValueType<C>>, T> Comp = Equal>
    constexpr Option<IteratorForContainer<C>> operator()(C&& container, const T& needle, Comp comparator = {}, Proj projection = {}) const {
        auto end = container.end();
        for (auto it = container.begin(); it != end; ++it) {
            if (invoke(comparator, invoke(projection, *it), needle)) {
                return it;
            }
        }
        return None {};
    }
};

constexpr inline auto find = FindFunction {};
}
