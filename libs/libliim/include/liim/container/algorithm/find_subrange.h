#pragma once

#include <liim/container/algorithm/mismatch.h>
#include <liim/container/concepts.h>
#include <liim/container/producer/iterator_container.h>
#include <liim/option.h>
#include <liim/tuple.h>

namespace LIIM::Container::Algorithm {
struct FindSubrangeFunction {
    template<Container C, Container D, ProjectionFor<ContainerValueType<C>> ProjC = Identity,
             ProjectionFor<ContainerValueType<D>> ProjD = Identity,
             ComparatorFor<Projected<ProjC, ContainerValueType<C>>, Projected<ProjD, ContainerValueType<D>>> Comp = Equal>
    constexpr Option<decltype(iterator_container(declval<IteratorForContainer<C>>(), declval<IteratorForContainer<C>>()))>
    operator()(C&& a, D&& b, Comp comparator = {}, ProjC projection_a = {}, ProjD projection_b = {}) const {
        if constexpr (SizedContainer<C> && SizedContainer<D>) {
            if (a.size() < b.size()) {
                return None {};
            }
        }
        auto a_end = a.end();
        auto b_end = b.end();

        auto it = a.begin();
        for (; it != a_end; ++it) {
            auto [at, bt] = Alg::mismatch(iterator_container(it, a_end), b, comparator, projection_a, projection_b);
            if (bt == b_end) {
                return iterator_container(it, at);
            }
        }
        return None {};
    }
};

constexpr inline auto find_subrange = FindSubrangeFunction {};
}
