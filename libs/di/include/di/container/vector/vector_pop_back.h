#pragma once

#include "di/container/vector/mutable_vector.h"
#include "di/container/vector/vector_lookup.h"
#include "di/container/vector/vector_size.h"
#include "di/util/move.h"
#include "di/util/relocate.h"
#include "di/vocab/optional/prelude.h"

namespace di::container::vector {
constexpr auto pop_back(concepts::detail::MutableVector auto& vector) {
    auto size = vector::size(vector);
    return lift_bool(size > 0) % [&] {
        auto new_size = size - 1;
        auto result = util::relocate(vector::lookup(vector, new_size));
        vector.assume_size(new_size);
        return result;
    };
}
}
