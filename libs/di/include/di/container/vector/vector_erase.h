#pragma once

#include <di/container/algorithm/destroy.h>
#include <di/container/algorithm/move.h>
#include <di/container/vector/mutable_vector.h>
#include <di/container/vector/vector_end.h>
#include <di/container/vector/vector_iterator.h>
#include <di/container/vector/vector_lookup.h>
#include <di/util/swap.h>

namespace di::container::vector {
template<concepts::detail::MutableVector Vec, typename Iter = meta::detail::VectorIterator<Vec>,
         typename CIter = meta::detail::VectorConstIterator<Vec>>
constexpr auto erase(Vec& vector, CIter cstart, CIter cend) -> Iter {
    auto start = vector::iterator(vector, cstart);
    auto end = vector::iterator(vector, cend);
    auto size = vector::size(vector);

    auto count = end - start;
    auto [old_end, new_end] = container::move(end, vector::end(vector), start);

    container::destroy(new_end, old_end);
    vector.assume_size(size - count);
    return start;
}

template<concepts::detail::MutableVector Vec, typename Iter = meta::detail::VectorIterator<Vec>,
         typename CIter = meta::detail::VectorConstIterator<Vec>>
constexpr auto erase(Vec& vector, CIter citerator) -> Iter {
    return vector::erase(vector, citerator, citerator + 1);
}
}
