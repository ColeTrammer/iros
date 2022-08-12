#pragma once

#include <liim/container/algorithm/find_subrange.h>
#include <liim/container/concepts.h>

namespace LIIM::Container::Algorithm {
struct ContainsSubrangeFunction {
    template<Container C, Container D, ProjectionFor<ContainerValueType<C>> ProjC = Identity,
             ProjectionFor<ContainerValueType<D>> ProjD = Identity,
             ComparatorFor<Projected<ProjC, ContainerValueType<C>>, Projected<ProjD, ContainerValueType<D>>> Comp = Equal>
    constexpr bool operator()(C&& a, D&& b, Comp comparator = {}, ProjC projection_a = {}, ProjD projection_b = {}) const {
        return !!Alg::find_subrange(forward<C>(a), forward<D>(b), move(comparator), move(projection_a), move(projection_b));
    }
};

constexpr inline auto contains_subrange = ContainsSubrangeFunction {};
}
