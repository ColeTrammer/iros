#pragma once

#include <di/concepts/constructible_from.h>
#include <di/concepts/copy_constructible.h>
#include <di/concepts/move_constructible.h>
#include <di/concepts/remove_cvref_same_as.h>
#include <di/container/vector/mutable_vector.h>
#include <di/container/vector/vector_data.h>
#include <di/container/vector/vector_reserve.h>
#include <di/container/vector/vector_size.h>
#include <di/util/construct_at.h>
#include <di/util/forward.h>
#include <di/vocab/expected/prelude.h>

namespace di::container::vector {
template<typename Vec = concepts::detail::MutableVector, concepts::RemoveCVRefSameAs<meta::detail::VectorValue<Vec>> U>
requires(concepts::ConstructibleFrom<meta::detail::VectorValue<Vec>, U>)
constexpr decltype(auto) push_back(Vec& vector, U&& value) {
    auto size = vector::size(vector);
    vector::reserve(vector, size + 1);
    auto end = vector::data(vector) + size;
    auto result = util::construct_at(end, util::forward<U>(value));
    vector.assume_size(size + 1);
    return *result;
}
}
