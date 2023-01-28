#pragma once

#include <di/container/vector/constant_vector.h>
#include <di/types/prelude.h>

namespace di::container::vector {
constexpr size_t size(concepts::detail::ConstantVector auto const& vector) {
    return vector.span().size();
}
}
