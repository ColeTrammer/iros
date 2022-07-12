#pragma once

#include <liim/container/concepts.h>

namespace LIIM::Container::Iterators {
template<MutableIterator Iter>
constexpr void swap_iterator_contents(Iter a, Iter b) {
    if constexpr (MemberContentSwappableIterator<Iter>) {
        a.swap_contents(b);
    } else {
        ::swap(*a, *b);
    }
}
}

using LIIM::Container::Iterators::swap_iterator_contents;
