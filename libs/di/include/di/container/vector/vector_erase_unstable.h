#pragma once

#include <di/container/vector/mutable_vector.h>
#include <di/container/vector/vector_end.h>
#include <di/container/vector/vector_iterator.h>
#include <di/container/vector/vector_lookup.h>
#include <di/util/destroy_at.h>
#include <di/util/swap.h>

namespace di::container::vector {
template<concepts::detail::MutableVector Vec, typename Iter = meta::detail::VectorIterator<Vec>,
         typename CIter = meta::detail::VectorConstIterator<Vec>>
constexpr auto erase_unstable(Vec& vector, CIter citerator) -> Iter {
    auto last = vector::end(vector) - 1;
    auto iterator = vector::iterator(vector, citerator);
    util::swap(*iterator, *last);
    util::destroy_at(last);
    return iterator;
}
}
