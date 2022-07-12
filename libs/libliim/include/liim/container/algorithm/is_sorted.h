#pragma once

#include <liim/container/concepts.h>
#include <liim/option.h>

namespace LIIM::Container::Algorithm {
template<Container C, ThreeWayComparatorFor<ContainerValueType<C>> Comp = CompareThreeWay>
constexpr bool is_sorted(C&& container, Comp comparator = {}) {
    auto start = container.begin();
    auto end = container.end();
    if (start == end) {
        return true;
    }

    // NOTE: use Option<> so that if the container type is a
    //       reference, we can safely rebind it.
    using Storage = Option<ContainerValueType<C>>;
    auto prev = Storage(*start);
    while (++start != end) {
        auto current = Storage(*start);
        if (invoke(comparator, *prev, *current) > 0) {
            return false;
        }
        prev = move(current);
    }
    return true;
}
}

using LIIM::Container::Algorithm::is_sorted;
