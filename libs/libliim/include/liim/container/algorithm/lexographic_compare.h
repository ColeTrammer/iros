#pragma once

#include <liim/container/concepts.h>

namespace LIIM::Container::Algorithm {
template<Container C, Container D, ThreeWayComparatorFor<ContainerValueType<C>, ContainerValueType<D>> Comp = CompareThreeWay>
constexpr typename InvokeResult<Comp, ContainerValueType<C>, ContainerValueType<D>>::type lexographic_compare(C&& a, D&& b,
                                                                                                              Comp comparator = {}) {
    auto a_end = a.end();
    auto b_end = b.end();

    auto it = a.begin();
    auto jt = b.begin();
    for (; it != a_end && jt != b_end; ++it, ++jt) {
        if (auto result = invoke(comparator, *it, *jt); result != 0) {
            return result;
        }
    }
    return (jt == b_end) <=> (it == a_end);
}
}

using LIIM::Container::Algorithm::lexographic_compare;
