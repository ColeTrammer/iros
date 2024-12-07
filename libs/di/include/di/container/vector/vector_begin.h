#pragma once

#include "di/container/vector/constant_vector.h"
#include "di/container/vector/vector_data.h"

namespace di::container::vector {
constexpr auto begin(concepts::detail::ConstantVector auto& vector) {
    return vector::data(vector);
}
}
