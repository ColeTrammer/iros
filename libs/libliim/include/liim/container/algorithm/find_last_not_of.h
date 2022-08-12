#pragma once

#include <liim/container/algorithm/find_first_not_of.h>
#include <liim/container/concepts.h>
#include <liim/container/view/reversed.h>
#include <liim/option.h>

namespace LIIM::Container::Algorithm {
struct FindLastNotOf {
    template<DoubleEndedContainer C, Container D, ProjectionFor<ContainerValueType<C>> ProjC = Identity,
             ProjectionFor<ContainerValueType<D>> ProjD = Identity,
             ComparatorFor<Projected<ProjC, ContainerValueType<C>>, Projected<ProjD, ContainerValueType<D>>> Comp = Equal>
    constexpr Option<IteratorForContainer<C>> operator()(C&& a, D&& b, Comp comparator = {}, ProjC projection_a = {},
                                                         ProjD projection_b = {}) const {
        return Alg::find_first_not_of(reversed(forward<C>(a)), forward<D>(b), move(comparator), move(projection_a), move(projection_b))
            .map([](auto it) {
                return (++it).base();
            });
    }
};

constexpr inline auto find_last_not_of = FindLastNotOf {};
}
