#pragma once

namespace di::types {
template<auto value>
struct Nontype {
    explicit Nontype() = default;
};

template<auto value>
constexpr inline auto nontype = Nontype<value> {};
}
