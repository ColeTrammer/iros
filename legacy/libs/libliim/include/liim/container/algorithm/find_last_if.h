#pragma once

#include <liim/container/algorithm/find_if_not.h>
#include <liim/container/concepts.h>
#include <liim/container/view/reversed.h>

namespace LIIM::Container::Algorithm {
struct FindLastIfFunction {
    template<Container C, ProjectionFor<ContainerValueType<C>> Proj = Identity, PredicateFor<Projected<Proj, ContainerValueType<C>>> Pred>
    constexpr Option<IteratorForContainer<C>> operator()(C&& container, Pred predicate, Proj projection = {}) const {
        return Alg::find_if(reversed(forward<C>(container)), move(predicate), move(projection)).map([](auto it) {
            return (++it).base();
        });
    }
};

constexpr inline auto find_last_if = FindLastIfFunction {};
}
