#pragma once

#include <di/container/concepts/bidirectional_iterator.h>
#include <di/container/iterator/advance.h>
#include <di/container/meta/iterator_ssize_type.h>

namespace di::container {
struct PrevFunction {
    template<concepts::BidirectionalIterator Iter>
    constexpr Iter operator()(Iter iterator) const {
        --iterator;
        return iterator;
    }

    template<concepts::BidirectionalIterator Iter>
    constexpr Iter operator()(Iter iterator, meta::IteratorSSizeType<Iter> n) const {
        container::advance(iterator, -n);
        return iterator;
    }

    template<concepts::BidirectionalIterator Iter>
    constexpr Iter operator()(Iter iterator, meta::IteratorSSizeType<Iter> n, Iter bound) const {
        container::advance(iterator, -n, bound);
        return iterator;
    }
};

constexpr inline auto prev = PrevFunction {};
}
