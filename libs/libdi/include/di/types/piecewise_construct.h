#pragma once

namespace di::types {
struct PiecewiseConstruct {
    explicit PiecewiseConstruct() = default;
};

constexpr inline auto piecewise_construct = PiecewiseConstruct {};
}
