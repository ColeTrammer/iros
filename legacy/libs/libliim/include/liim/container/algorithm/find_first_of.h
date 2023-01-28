#pragma once

#include <liim/container/algorithm/contains.h>
#include <liim/container/algorithm/find_if.h>
#include <liim/container/concepts.h>
#include <liim/option.h>

namespace LIIM::Container::Algorithm {
struct FindFirstOf {
    template<Container C, Container D, ProjectionFor<ContainerValueType<C>> ProjC = Identity,
             ProjectionFor<ContainerValueType<D>> ProjD = Identity,
             ComparatorFor<Projected<ProjC, ContainerValueType<C>>, Projected<ProjD, ContainerValueType<D>>> Comp = Equal>
    constexpr Option<IteratorForContainer<C>> operator()(C&& a, D&& b, Comp comparator = {}, ProjC projection_a = {},
                                                         ProjD projection_b = {}) const {
        return Alg::find_if(
            forward<C>(a),
            [&](auto&& element) {
                return Alg::contains(forward<D>(b), element, comparator, projection_b);
            },
            projection_a);
    }
};

constexpr inline auto find_first_of = FindFirstOf {};
}
