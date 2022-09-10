#pragma once

#include <di/container/vector/constant_vector.h>
#include <di/container/vector/vector_data.h>
#include <di/container/vector/vector_size.h>
#include <di/types/prelude.h>

namespace di::container::vector {
constexpr decltype(auto) lookup(concepts::detail::ConstantVector auto& vector, size_t index) {
    // DI_ASSERT( index < vector::size(vector) )
    return vector::data(vector)[index];
}
}
