#pragma once

#include <di/container/vector/mutable_vector.h>
#include <di/container/vector/vector_data.h>
#include <di/container/vector/vector_size.h>
#include <di/types/prelude.h>
#include <di/util/swap.h>
#include <di/util/uninitialized_relocate.h>

namespace di::container::vector {
template<typename Vec = concepts::detail::MutableVector>
constexpr void reserve(Vec& vector, size_t size) {
    if (size <= vector.capacity()) {
        return;
    }

    auto temp = Vec();
    temp.reserve_from_nothing(size);
    auto new_buffer = vector::data(temp);
    util::uninitialized_relocate(vector.begin(), vector.end(), new_buffer, new_buffer + size);
    vector.assume_size(0);
    util::swap(vector, temp);
}
}
