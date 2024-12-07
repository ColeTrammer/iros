#pragma once

#include "diusgfx/point.h"
#include "diusgfx/size2d.h"

namespace gfx {
class Rect {
public:
    Rect() = default;
    constexpr Rect(f32 x, f32 y, f32 width, f32 height) : m_top_left(x, y), m_size(width, height) {}
    constexpr Rect(Point top_left, Size2d size) : m_top_left(top_left), m_size(size) {}

    constexpr auto x() -> f32& { return m_top_left.x(); }
    constexpr auto x() const -> f32 { return m_top_left.x(); }

    constexpr auto y() -> f32& { return m_top_left.y(); }
    constexpr auto y() const -> f32 { return m_top_left.y(); }

    constexpr auto width() -> f32& { return m_size.width(); }
    constexpr auto width() const -> f32 { return m_size.width(); }

    constexpr auto height() -> f32& { return m_size.height(); }
    constexpr auto height() const -> f32 { return m_size.height(); }

    constexpr auto top_left() -> Point& { return m_top_left; }
    constexpr auto top_left() const -> Point { return m_top_left; }

    constexpr auto size() -> Size2d& { return m_size; }
    constexpr auto size() const -> Size2d { return m_size; }

    constexpr auto left() const -> f32 { return x(); }
    constexpr auto right() const -> f32 { return x() + width(); }
    constexpr auto top() const -> f32 { return y(); }
    constexpr auto bottom() const -> f32 { return y() + height(); }

    constexpr auto with_x(f32 x) const -> Rect { return { x, y(), width(), height() }; }
    constexpr auto with_y(f32 y) const -> Rect { return { x(), y, width(), height() }; }
    constexpr auto with_top_left(Point p) const -> Rect { return { p, size() }; }

    constexpr auto with_width(f32 width) const -> Rect { return { x(), y(), width, height() }; }
    constexpr auto with_height(f32 height) const -> Rect { return { x(), y(), width(), height }; }
    constexpr auto with_size(Size2d size) const -> Rect { return { top_left(), size }; }

    constexpr auto top_right() const -> Point { return top_left().with_x(right()); }
    constexpr auto bottom_left() const -> Point { return top_left().with_y(bottom()); }
    constexpr auto bottom_right() const -> Point { return top_right().with_y(bottom()); }

    constexpr auto center() const -> Point {
        auto result = top_left();
        result.x() += size().width() / 2;
        result.y() += size().height() / 2;
        return result;
    }

    constexpr auto contains(Point p) const -> bool {
        auto [x, y] = p;
        return x >= top_left().x() && x <= bottom_right().x() && y >= top_left().y() && y <= bottom_right().y();
    }
    constexpr auto contains(Rect other) const -> bool { return contains(other.top_left()) && contains(bottom_right()); }

    auto operator==(Rect const&) const -> bool = default;

private:
    Point m_top_left;
    Size2d m_size;
};
}
