#pragma once

#include <di/container/vector/constant_vector.h>

namespace di::container::vector {
constexpr auto data(concepts::detail::ConstantVector auto& vector) {
    return vector.span().data();
}
}
