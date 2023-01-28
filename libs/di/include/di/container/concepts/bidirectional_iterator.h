#pragma once

#include <di/container/concepts/forward_iterator.h>
#include <di/container/types/bidirectional_iterator_tag.h>

namespace di::concepts {
template<typename Iter>
concept BidirectionalIterator =
    ForwardIterator<Iter> && DerivedFrom<meta::IteratorCategory<Iter>, types::BidirectionalIteratorTag> &&
    requires(Iter iter) {
        { --iter } -> SameAs<Iter&>;
        { iter-- } -> SameAs<Iter>;
    };
}
