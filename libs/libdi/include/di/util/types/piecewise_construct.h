#pragma once

namespace di::util::types {
struct PiecewiseConstruct {
    explicit PiecewiseConstruct() = default;
};

constexpr inline auto piecewise_construct = PiecewiseConstruct {};
}
