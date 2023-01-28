#pragma once

namespace di::types {
struct InPlace {
    explicit InPlace() = default;
};

constexpr inline auto in_place = InPlace {};
}
