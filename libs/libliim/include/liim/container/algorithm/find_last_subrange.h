#pragma once

#include <liim/container/algorithm/find_subrange.h>
#include <liim/container/concepts.h>
#include <liim/container/view/reversed.h>

namespace LIIM::Container::Algorithm {
struct FindLastSubrangeFunction {
    template<Container C, Container D, ProjectionFor<ContainerValueType<C>> ProjC = Identity,
             ProjectionFor<ContainerValueType<D>> ProjD = Identity,
             ComparatorFor<Projected<ProjC, ContainerValueType<C>>, Projected<ProjD, ContainerValueType<D>>> Comp = Equal>
    constexpr Option<decltype(iterator_container(declval<IteratorForContainer<C>>(), declval<IteratorForContainer<C>>()))>
    operator()(C&& a, D&& b, Comp comparator = {}, ProjC projection_a = {}, ProjD projection_b = {}) const {
        return Alg::find_subrange(reversed(forward<C>(a)), reversed(forward<D>(b)), move(comparator), move(projection_a),
                                  move(projection_b))
            .map([](auto subrange) {
                return iterator_container(subrange.end().base(), subrange.begin().base());
            });
    }
};

constexpr inline auto find_last_subrange = FindLastSubrangeFunction {};
}
