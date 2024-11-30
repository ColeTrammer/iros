#pragma once

#include <di/math/linalg/vec.h>
#include <di/types/floats.h>
#include <di/types/integers.h>
#include <di/util/get.h>

namespace gfx {
struct Size2dType {
    using Type = f32;
    constexpr static auto extent = 2ZU;

    struct Mixin {
        using Self = di::math::linalg::Vec<Size2dType>;

        constexpr auto width() const { return di::get<0>(as_self()); }
        constexpr auto width() -> f32& { return di::get<0>(as_self()); }

        constexpr auto height() const { return di::get<1>(as_self()); }
        constexpr auto height() -> f32& { return di::get<1>(as_self()); }

        constexpr auto with_width(Type width) const { return Self { width, height() }; }
        constexpr auto with_height(Type height) const { return Self { width(), height }; }

    private:
        constexpr auto as_self() const -> Self const& { return static_cast<Self const&>(*this); }
        constexpr auto as_self() -> Self& { return static_cast<Self&>(*this); }
    };
};

using Size2d = di::math::linalg::Vec<Size2dType>;
}
