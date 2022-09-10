#pragma once

#include <di/container/vector/constant_vector.h>
#include <di/container/vector/vector_size.h>
#include <di/types/prelude.h>

namespace di::container::vector {
template<concepts::detail::ConstantVector Vec>
constexpr size_t size_bytes(Vec const& vector) {
    return sizeof(meta::detail::VectorValue<Vec>) * vector::size(vector);
}
}
