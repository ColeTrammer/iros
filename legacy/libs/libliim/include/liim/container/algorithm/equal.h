#pragma once

#include <liim/container/concepts.h>

namespace LIIM::Container::Algorithm {
struct EqualFunction {
    template<Container C, Container D, ProjectionFor<ContainerValueType<C>> ProjC = Identity,
             ProjectionFor<ContainerValueType<D>> ProjD = Identity,
             ComparatorFor<Projected<ProjC, ContainerValueType<C>>, Projected<ProjD, ContainerValueType<D>>> Comp = Equal>
    constexpr bool operator()(C&& a, D&& b, Comp comparator = {}, ProjC projection_a = {}, ProjD projection_b = {}) const {
        if constexpr (SizedContainer<C> && SizedContainer<D>) {
            if (a.size() != b.size()) {
                return false;
            }
        }
        auto a_end = a.end();
        auto b_end = b.end();

        auto it = a.begin();
        auto jt = b.begin();
        for (; it != a_end && jt != b_end; ++it, ++jt) {
            if (!invoke(comparator, invoke(projection_a, *it), invoke(projection_b, *jt))) {
                return false;
            }
        }
        return it == a_end && jt == b_end;
    }
};

constexpr inline auto equal = EqualFunction {};
}
