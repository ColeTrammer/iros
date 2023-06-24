#pragma once

#include <di/container/view/adjacent_transform.h>

namespace di::container::view {
constexpr inline auto pairwise_transform = view::adjacent_transform<2>;
}

namespace di {
using view::pairwise_transform;
}
