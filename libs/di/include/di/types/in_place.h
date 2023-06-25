#pragma once

namespace di::types {
struct InPlace {
    explicit InPlace() = default;
};

constexpr inline auto in_place = InPlace {};
}

namespace di {
using types::in_place;
using types::InPlace;
}
