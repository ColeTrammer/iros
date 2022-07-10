#pragma once

#include <liim/container/concepts.h>

namespace LIIM::Container::Algorithm {
template<Container C, Container D, ComparatorFor<ContainerValueType<C>, ContainerValueType<D>> Comp = Equal>
constexpr bool equal(C&& a, D&& b, Comp comparator = {}) {
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
        if (!invoke(comparator, *it, *jt)) {
            return false;
        }
    }
    return it == a_end && jt == b_end;
}
}

using LIIM::Container::Algorithm::equal;
