#pragma once

#include <di/container/vector/constant_vector.h>
#include <di/container/vector/vector_empty.h>
#include <di/container/vector/vector_lookup.h>
#include <di/container/vector/vector_size.h>
#include <di/util/reference_wrapper.h>
#include <di/vocab/optional/prelude.h>

namespace di::container::vector {
constexpr auto back(concepts::detail::ConstantVector auto& vector) {
    return lift_bool(!vector::empty(vector)) % [&] {
        return util::ref(vector::lookup(vector, vector::size(vector) - 1));
    };
}
}
