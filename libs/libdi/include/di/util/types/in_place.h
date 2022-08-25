#pragma once

namespace di::util::types {
struct InPlace {
    explicit InPlace() = default;
};

constexpr inline auto in_place = InPlace {};
}
