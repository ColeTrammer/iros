#pragma once

namespace di::types {
template<auto value>
struct InPlaceValue {
    explicit InPlaceValue() = default;
};

template<auto value>
constexpr inline auto in_place_value = InPlaceValue<value> {};
}