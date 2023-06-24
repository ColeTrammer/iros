#pragma once

#include <di/container/concepts/iterator.h>
#include <di/container/concepts/random_access_iterator.h>
#include <di/container/concepts/sentinel_for.h>
#include <di/container/meta/iterator_ssize_type.h>
#include <di/meta/operations.h>
#include <di/util/move.h>

namespace di::container {
struct AdvanceFunction {
    template<concepts::Iterator Iter>
    constexpr void operator()(Iter& iterator, meta::IteratorSSizeType<Iter> n) const {
        if constexpr (concepts::RandomAccessIterator<Iter>) {
            iterator += n;
        } else {
            for (; n > 0; --n, ++iterator) {}
            if constexpr (concepts::BidirectionalIterator<Iter>) {
                for (; n < 0; ++n, --iterator) {}
            }
        }
    }

    template<concepts::Iterator Iter, concepts::SentinelFor<Iter> Sent>
    constexpr void operator()(Iter& iterator, Sent bound) const {
        if constexpr (concepts::AssignableFrom<Iter&, Sent>) {
            iterator = util::move(bound);
        } else if constexpr (concepts::SizedSentinelFor<Sent, Iter> && concepts::RandomAccessIterator<Iter>) {
            iterator += (bound - iterator);
        } else {
            for (; iterator != bound; ++iterator) {}
        }
    }

    template<concepts::Iterator Iter, concepts::SentinelFor<Iter> Sent>
    constexpr meta::IteratorSSizeType<Iter> operator()(Iter& iterator, meta::IteratorSSizeType<Iter> n,
                                                       Sent bound) const {
        if constexpr (concepts::SizedSentinelFor<Sent, Iter>) {
            if constexpr (concepts::BidirectionalIterator<Iter>) {
                if (n < 0) {
                    auto bounded_distance = (iterator - bound);
                    auto distance = bounded_distance > -n ? -n : bounded_distance;
                    (*this)(iterator, -distance);
                    return n + distance;
                }
            }

            auto bounded_distance = (bound - iterator);
            auto distance = bounded_distance > n ? n : bounded_distance;
            (*this)(iterator, distance);
            return n - bounded_distance;
        } else {
            for (; n > 0 && iterator != bound; --n, ++iterator) {}
            if constexpr (concepts::BidirectionalIterator<Iter>) {
                for (; n < 0 && iterator != bound; ++n, --iterator) {}
            }
            return n;
        }
    }
};

constexpr inline auto advance = AdvanceFunction {};
}

namespace di {
using container::advance;
}
