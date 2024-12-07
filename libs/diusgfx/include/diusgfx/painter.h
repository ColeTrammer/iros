#pragma once

#include "di/any/container/any.h"
#include "di/any/dispatch/dispatcher.h"
#include "di/any/types/this.h"
#include "di/meta/core.h"
#include "diusgfx/bitmap.h"
#include "diusgfx/color.h"
#include "diusgfx/point.h"
#include "diusgfx/rect.h"

namespace gfx {
namespace painter {
    struct DrawRect : di::Dispatcher<DrawRect, void(di::This&, Rect, Color)> {};
    struct DrawCircle : di::Dispatcher<DrawCircle, void(di::This&, Point, float, Color)> {};

    using PainterInterface = di::meta::List<DrawRect, DrawCircle>;
};

using Painter = di::Any<painter::PainterInterface>;

constexpr inline auto draw_rect = painter::DrawRect {};
constexpr inline auto draw_circle = painter::DrawCircle {};

auto make_painter(ExclusiveBitMap bitmap) -> Painter;
}
