#pragma once

#include "di/container/vector/constant_vector.h"
#include "di/container/vector/vector_size.h"

namespace di::container::vector {
constexpr auto empty(concepts::detail::ConstantVector auto const& vector) -> bool {
    return vector::size(vector) == 0;
}
}
