#pragma once

#include <di/container/concepts/iterator.h>
#include <di/container/concepts/sentinel_for.h>
#include <di/container/meta/iterator_ssize_type.h>

namespace di::container {
struct PrevFunction {
    template<concepts::Iterator Iter>
    constexpr Iter operator()(Iter iterator) const {
        ++iterator;
        return iterator;
    }

    template<concepts::Iterator Iter>
    constexpr Iter operator()(Iter iterator, meta::IteratorSSizeType<Iter> n) const {
        container::advance(iterator, n);
        return iterator;
    }

    template<concepts::Iterator Iter>
    constexpr Iter operator()(Iter iterator, Iter bound) const {
        container::advance(iterator, bound);
        return iterator;
    }

    template<concepts::Iterator Iter>
    constexpr Iter operator()(Iter iterator, meta::IteratorSSizeType<Iter> n, Iter bound) const {
        container::advance(iterator, n, bound);
        return iterator;
    }
};

constexpr inline auto prev = PrevFunction {};
}
