#pragma once

#include "di/container/vector/constant_vector.h"
#include "di/types/prelude.h"

namespace di::container::vector {
constexpr auto size(concepts::detail::ConstantVector auto const& vector) -> size_t {
    return vector.span().size();
}
}
