#pragma once

#include <liim/container/algorithm/starts_with.h>
#include <liim/container/concepts.h>
#include <liim/container/view/reversed.h>

namespace LIIM::Container::Algorithm {
struct EndsWithFunction {
    template<DoubleEndedContainer C, DoubleEndedContainer D, ProjectionFor<ContainerValueType<C>> ProjC = Identity,
             ProjectionFor<ContainerValueType<D>> ProjD = Identity,
             ComparatorFor<Projected<ProjC, ContainerValueType<C>>, Projected<ProjD, ContainerValueType<D>>> Comp = Equal>
    constexpr bool operator()(C&& a, D&& b, Comp comparator = {}, ProjC projection_a = {}, ProjD projection_b = {}) const {
        return Alg::starts_with(reversed(forward<C>(a)), reversed(forward<D>(b)), move(comparator), move(projection_a), move(projection_b));
    }
};

constexpr inline auto ends_with = EndsWithFunction {};
}
