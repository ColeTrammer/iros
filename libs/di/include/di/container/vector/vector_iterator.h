#pragma once

#include <di/assert/assert_bool.h>
#include <di/container/vector/constant_vector.h>
#include <di/container/vector/vector_begin.h>
#include <di/container/vector/vector_size.h>
#include <di/meta/language.h>

namespace di::container::vector {
template<concepts::detail::ConstantVector Vec, typename Iter = meta::detail::VectorIterator<Vec>,
         typename CIter = meta::detail::VectorConstIterator<Vec>>
requires(!concepts::Const<Vec>)
constexpr auto iterator(Vec&, CIter iterator) -> Iter {
    return const_cast<Iter>(iterator);
}

template<concepts::detail::ConstantVector Vec, typename Iter = meta::detail::VectorIterator<Vec>>
requires(!concepts::Const<Vec>)
constexpr auto iterator(Vec& vector, size_t index) -> Iter {
    DI_ASSERT(index <= vector::size(vector));
    return vector::begin(vector) + index;
}

template<concepts::detail::ConstantVector Vec, typename CIter = meta::detail::VectorConstIterator<Vec>>
constexpr auto iterator(Vec const& vector, size_t index) -> CIter {
    DI_ASSERT(index <= vector::size(vector));
    return vector::begin(vector) + index;
}
}
