#pragma once

#include <liim/container/algorithm/find.h>
#include <liim/container/concepts.h>
#include <liim/container/view/reversed.h>

namespace LIIM::Container::Algorithm {
struct FindLastFunction {
    template<DoubleEndedContainer C, typename T = ContainerValueType<C>, ProjectionFor<ContainerValueType<C>> Proj = Identity,
             ComparatorFor<Projected<Proj, ContainerValueType<C>>, T> Comp = Equal>
    constexpr Option<IteratorForContainer<C>> operator()(C&& container, const T& needle, Comp comparator = {}, Proj projection = {}) const {
        return Alg::find(reversed(forward<C>(container)), needle, move(comparator), move(projection)).map([](auto it) {
            return (++it).base();
        });
    }
};

constexpr inline auto find_last = FindLastFunction {};
}
