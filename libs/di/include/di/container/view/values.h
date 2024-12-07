#pragma once

#include "di/container/view/elements.h"

namespace di::container::view {
constexpr inline auto values = elements<1>;
}

namespace di {
using view::values;
}
