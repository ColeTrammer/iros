#pragma once

#include <liim/container/concepts.h>

namespace LIIM::Container::Algorithm {
template<Container C, typename T = ContainerValueType<C>, ComparatorFor<ContainerValueType<C>, T> Comp = Equal>
constexpr bool contains(C&& container, const T& needle, Comp comparator = {}) {
    auto end = container.end();
    for (auto it = container.begin(); it != end; ++it) {
        if (invoke(comparator, *it, needle)) {
            return true;
        }
    }
    return false;
}
}

using LIIM::Container::Algorithm::contains;
