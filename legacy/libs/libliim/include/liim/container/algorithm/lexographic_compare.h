#pragma once

#include <liim/container/concepts.h>

namespace LIIM::Container::Algorithm {
struct LexographicCompareFunction {
    template<Container C, Container D, ProjectionFor<ContainerValueType<C>> ProjC = Identity,
             ProjectionFor<ContainerValueType<D>> ProjD = Identity,
             ThreeWayComparatorFor<Projected<ProjC, ContainerValueType<C>>, Projected<ProjD, ContainerValueType<D>>> Comp = CompareThreeWay>
    constexpr typename InvokeResult<Comp, Projected<ProjC, ContainerValueType<C>>, Projected<ProjD, ContainerValueType<D>>>::type
    operator()(C&& a, D&& b, Comp comparator = {}, ProjC projection_a = {}, ProjD projection_b = {}) const {
        auto a_end = a.end();
        auto b_end = b.end();

        auto it = a.begin();
        auto jt = b.begin();
        for (; it != a_end && jt != b_end; ++it, ++jt) {
            if (auto result = invoke(comparator, invoke(projection_a, *it), invoke(projection_b, *jt)); result != 0) {
                return result;
            }
        }
        return (jt == b_end) <=> (it == a_end);
    }
};

constexpr inline auto lexographic_compare = LexographicCompareFunction {};
}
