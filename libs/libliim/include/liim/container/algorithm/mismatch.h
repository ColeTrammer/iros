#pragma once

#include <liim/container/concepts.h>
#include <liim/tuple.h>

namespace LIIM::Container::Algorithm {
struct MismatchFunction {
    template<Container C, Container D, ProjectionFor<ContainerValueType<C>> ProjC = Identity,
             ProjectionFor<ContainerValueType<D>> ProjD = Identity,
             ComparatorFor<Projected<ProjC, ContainerValueType<C>>, Projected<ProjD, ContainerValueType<D>>> Comp = Equal>
    constexpr Tuple<IteratorForContainer<C>, IteratorForContainer<D>> operator()(C&& a, D&& b, Comp comparator = {},
                                                                                 ProjC projection_a = {}, ProjD projection_b = {}) const {
        auto a_end = a.end();
        auto b_end = b.end();

        auto it = a.begin();
        auto jt = b.begin();
        for (; it != a_end && jt != b_end; ++it, ++jt) {
            if (!invoke(comparator, invoke(projection_a, *it), invoke(projection_b, *jt))) {
                return { move(it), move(jt) };
            }
        }
        return { move(it), move(jt) };
    }
};

constexpr inline auto mismatch = MismatchFunction {};
}
