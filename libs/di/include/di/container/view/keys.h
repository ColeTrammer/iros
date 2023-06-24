#pragma once

#include <di/container/view/elements.h>

namespace di::container::view {
constexpr inline auto keys = elements<0>;
}

namespace di {
using view::keys;
}
