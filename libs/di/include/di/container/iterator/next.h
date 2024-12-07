#pragma once

#include "di/container/concepts/iterator.h"
#include "di/container/concepts/sentinel_for.h"
#include "di/container/iterator/advance.h"
#include "di/container/meta/iterator_ssize_type.h"

namespace di::container {
struct NextFunction {
    template<concepts::Iterator Iter>
    constexpr auto operator()(Iter iterator) const -> Iter {
        ++iterator;
        return iterator;
    }

    template<concepts::Iterator Iter>
    constexpr auto operator()(Iter iterator, meta::IteratorSSizeType<Iter> n) const -> Iter {
        container::advance(iterator, n);
        return iterator;
    }

    template<concepts::Iterator Iter, concepts::SentinelFor<Iter> Sent>
    constexpr auto operator()(Iter iterator, Sent bound) const -> Iter {
        container::advance(iterator, bound);
        return iterator;
    }

    template<concepts::Iterator Iter, concepts::SentinelFor<Iter> Sent>
    constexpr auto operator()(Iter iterator, meta::IteratorSSizeType<Iter> n, Sent bound) const -> Iter {
        container::advance(iterator, n, bound);
        return iterator;
    }
};

constexpr inline auto next = NextFunction {};
}

namespace di {
using container::next;
}
