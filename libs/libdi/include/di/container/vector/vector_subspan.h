#pragma once

#include <di/container/vector/constant_vector.h>
#include <di/container/vector/vector_data.h>
#include <di/container/vector/vector_size.h>
#include <di/types/prelude.h>
#include <di/vocab/optional/prelude.h>
#include <di/vocab/span/fixed_span.h>
#include <di/vocab/span/span_fixed_size.h>
#include <di/vocab/span/span_forward_declaration.h>

namespace di::container::vector {
constexpr auto subspan(concepts::detail::ConstantVector auto& vector, size_t offset) {
    auto size = vector::size(vector);
    return lift_bool(offset <= size) % [&] {
        return vocab::Span { vector::data(vector) + offset, size - offset };
    };
}

constexpr auto subspan(concepts::detail::ConstantVector auto& vector, size_t offset, size_t count) {
    auto size = vector::size(vector);
    return lift_bool(offset + count <= size) % [&] {
        return vocab::Span { vector::data(vector) + offset, count };
    };
}

template<size_t offset, size_t count = vocab::dynamic_extent>
constexpr auto subspan(concepts::detail::ConstantVector auto& vector) {
    if constexpr (count == vocab::dynamic_extent) {
        return vector::subspan(vector, offset);
    } else {
        return lift_bool(offset + count <= vector::size(vector)) % [&] {
            return vocab::fixed_span<count>(vector::data(vector) + offset);
        };
    }
}
}
