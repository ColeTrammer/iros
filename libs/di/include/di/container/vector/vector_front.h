#pragma once

#include "di/container/vector/constant_vector.h"
#include "di/container/vector/vector_empty.h"
#include "di/container/vector/vector_lookup.h"
#include "di/util/reference_wrapper.h"
#include "di/vocab/optional/prelude.h"

namespace di::container::vector {
constexpr auto front(concepts::detail::ConstantVector auto& vector) {
    return lift_bool(!vector::empty(vector)) % [&] {
        return util::ref(vector::lookup(vector, 0));
    };
}
}
