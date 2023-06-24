#pragma once

namespace di::types {
struct FromContainer {};

constexpr inline auto from_container = FromContainer {};
}

namespace di {
using types::from_container;
using types::FromContainer;
}
