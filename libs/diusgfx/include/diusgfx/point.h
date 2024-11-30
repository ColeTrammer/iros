#pragma once

#include <di/math/linalg/vec.h>
#include <di/types/floats.h>
#include <di/types/integers.h>
#include <di/util/get.h>

namespace gfx {
struct PointTag {
    using Type = f32;
    constexpr static auto extent = 2ZU;

    struct Mixin {
        using Self = di::math::linalg::Vec<PointTag>;

        constexpr auto x() const { return di::get<0>(as_self()); }
        constexpr auto x() -> f32& { return di::get<0>(as_self()); }

        constexpr auto y() const { return di::get<1>(as_self()); }
        constexpr auto y() -> f32& { return di::get<1>(as_self()); }

        constexpr auto with_x(Type x) const { return Self { x, y() }; }
        constexpr auto with_y(Type y) const { return Self { x(), y }; }

    private:
        constexpr auto as_self() const -> Self const& { return static_cast<Self const&>(*this); }
        constexpr auto as_self() -> Self& { return static_cast<Self&>(*this); }
    };
};

using Point = di::math::linalg::Vec<PointTag>;
}
