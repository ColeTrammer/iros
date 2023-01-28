#pragma once

#include <di/concepts/totally_ordered.h>
#include <di/container/concepts/bidirectional_iterator.h>
#include <di/container/concepts/sized_sentinel_for.h>
#include <di/container/meta/iterator_ssize_type.h>
#include <di/container/meta/iterator_value.h>
#include <di/container/types/random_access_iterator_tag.h>

namespace di::concepts {
template<typename Iter>
concept RandomAccessIterator =
    BidirectionalIterator<Iter> && DerivedFrom<meta::IteratorCategory<Iter>, types::RandomAccessIteratorTag> &&
    TotallyOrdered<Iter> && SizedSentinelFor<Iter, Iter> &&
    requires(Iter iterator, Iter const citerator, meta::IteratorSSizeType<Iter> const n) {
        { iterator += n } -> SameAs<Iter&>;
        { citerator + n } -> SameAs<Iter>;
        { n + citerator } -> SameAs<Iter>;
        { iterator -= n } -> SameAs<Iter&>;
        { citerator - n } -> SameAs<Iter>;
        { citerator[n] } -> SameAs<meta::IteratorReference<Iter>>;
    };
}
