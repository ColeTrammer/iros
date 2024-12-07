#pragma once

#include "di/container/vector/constant_vector.h"
#include "di/container/vector/vector_data.h"
#include "di/container/vector/vector_size.h"

namespace di::container::vector {
constexpr auto end(concepts::detail::ConstantVector auto& vector) {
    return vector::data(vector) + vector::size(vector);
}
}
