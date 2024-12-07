#pragma once

#include "di/container/vector/constant_vector.h"
#include "di/container/vector/vector_lookup.h"
#include "di/container/vector/vector_size.h"
#include "di/types/prelude.h"
#include "di/util/reference_wrapper.h"
#include "di/vocab/optional/prelude.h"

namespace di::container::vector {
constexpr auto at(concepts::detail::ConstantVector auto& vector, size_t index) {
    return lift_bool(index < vector::size(vector)) % [&] {
        return util::ref(vector::lookup(vector, index));
    };
}
}
