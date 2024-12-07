#pragma once

#include "di/container/vector/constant_vector.h"
#include "di/container/vector/vector_end.h"
#include "di/container/vector/vector_size.h"
#include "di/types/prelude.h"
#include "di/vocab/optional/prelude.h"
#include "di/vocab/span/fixed_span.h"
#include "di/vocab/span/span_fixed_size.h"
#include "di/vocab/span/span_forward_declaration.h"

namespace di::container::vector {
constexpr auto last(concepts::detail::ConstantVector auto& vector, size_t count) {
    return lift_bool(count <= vector::size(vector)) % [&] {
        return vocab::Span { vector::end(vector) - count, count };
    };
}

template<size_t count>
constexpr auto last(concepts::detail::ConstantVector auto& vector) {
    return lift_bool(count <= vector::size(vector)) % [&] {
        return vocab::fixed_span<count>(vector::end(vector) - count);
    };
}
}
