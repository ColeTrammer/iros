#pragma once

#include <di/container/concepts/iterator.h>
#include <di/container/concepts/sentinel_for.h>
#include <di/container/meta/iterator_ssize_type.h>

namespace di::container {
struct NextFunction {
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

    template<concepts::Iterator Iter, concepts::SentinelFor<Iter> Sent>
    constexpr Iter operator()(Iter iterator, Sent bound) const {
        container::advance(iterator, bound);
        return iterator;
    }

    template<concepts::Iterator Iter, concepts::SentinelFor<Iter> Sent>
    constexpr Iter operator()(Iter iterator, meta::IteratorSSizeType<Iter> n, Sent bound) const {
        container::advance(iterator, n, bound);
        return iterator;
    }
};

constexpr inline auto next = NextFunction {};
}
