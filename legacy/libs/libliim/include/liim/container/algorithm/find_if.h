#pragma once

#include <liim/container/concepts.h>
#include <liim/option.h>

namespace LIIM::Container::Algorithm {
struct FindIfFunction {
    template<Container C, ProjectionFor<ContainerValueType<C>> Proj = Identity, PredicateFor<Projected<Proj, ContainerValueType<C>>> Pred>
    constexpr Option<IteratorForContainer<C>> operator()(C&& container, Pred predicate, Proj projection = {}) const {
        auto end = container.end();
        for (auto it = container.begin(); it != end; ++it) {
            if (invoke(predicate, invoke(projection, *it))) {
                return it;
            }
        }
        return None {};
    }
};

constexpr inline auto find_if = FindIfFunction {};
}
