#pragma once

#include <graphics/point.h>
#include <liim/utilities.h>

class Rect {
public:
    constexpr Rect() {}
    constexpr Rect(int x, int y, int width, int height) : m_x(x), m_y(y), m_width(width), m_height(height) {}
    constexpr Rect(const Point& a, const Point& b)
        : m_x(min(a.x(), b.x())), m_y(min(a.y(), b.y())), m_width(abs(a.x() - b.x())), m_height(abs(a.y() - b.y())) {}

    constexpr int x() const { return m_x; }
    constexpr int y() const { return m_y; }
    constexpr int width() const { return m_width; }
    constexpr int height() const { return m_height; }

    constexpr void set_x(int x) { m_x = x; }
    constexpr void set_y(int y) { m_y = y; }
    constexpr void set_width(int width) { m_width = width; }
    constexpr void set_height(int height) { m_height = height; }

    constexpr Point top_left() const { return { x(), y() }; }
    constexpr Point top_right() const { return { x() + width(), y() }; }
    constexpr Point bottom_left() const { return { x(), y() + height() }; }
    constexpr Point bottom_right() const { return { x() + width(), y() + height() }; }
    constexpr Point center() const { return Point(x() + width() / 2, y() + height() / 2); }

    constexpr Rect top_edge() const { return { x(), y(), width(), 1 }; }
    constexpr Rect right_edge() const { return { x() + width() - 1, y(), 1, height() }; }
    constexpr Rect bottom_edge() const { return { x(), y() + height() - 1, width(), 1 }; }
    constexpr Rect left_edge() const { return { x(), y(), 1, height() }; }

    constexpr bool intersects(Point p) const { return p.x() >= m_x && p.x() <= m_x + m_width && p.y() >= m_y && p.y() <= m_y + m_height; }
    constexpr bool intersects(const Rect& other) const {
        return !(this->x() >= other.x() + other.width() || this->x() + this->width() <= other.x() ||
                 this->y() >= other.y() + other.height() || this->y() + this->height() <= other.y());
    }

    constexpr Rect intersection_with(const Rect& other) const {
        if (!intersects(other)) {
            return {};
        }

        auto x = max(this->x(), other.x());
        auto y = max(this->y(), other.y());
        auto end_x = min(this->x() + this->width(), other.x() + other.width());
        auto end_y = min(this->y() + this->height(), other.y() + other.height());
        return { x, y, end_x - x, end_y - y };
    }

    constexpr Rect adjusted(int d) const { return adjusted(d, d); }
    constexpr Rect adjusted(int dx, int dy) const { return { x() - dx, y() - dy, width() + 2 * dx, height() + 2 * dy }; }

    constexpr bool operator==(const Rect& other) const {
        return this->x() == other.x() && this->y() == other.y() && this->width() == other.width() && this->height() == other.height();
    };
    constexpr bool operator!=(const Rect& other) const { return !(*this == other); }

private:
    int m_x { 0 };
    int m_y { 0 };
    int m_width { 0 };
    int m_height { 0 };
};

static_assert(Rect(10, 10, 500, 500).intersects(Point(15, 15)));
static_assert(!Rect(10, 10, 500, 500).intersects(Rect()));
static_assert(!Rect(10, 10, 500, 500).intersects(Rect(550, 50, 5, 5)));
static_assert(Rect(10, 10, 500, 500).intersects(Rect(50, 50, 50, 50)));

static_assert(Rect(50, 50, 50, 50).intersection_with(Rect(60, 60, 10, 10)) == Rect(60, 60, 10, 10));
static_assert(Rect(100, 25, 50, 50).intersection_with(Rect(50, 35, 100, 10)) == Rect(100, 35, 50, 10));
static_assert(Rect(0, 0, 500, 500).intersection_with(Rect(50, 50, 500, 500)) == Rect(50, 50, 450, 450));
static_assert(Rect(50, 50, 50, 50).intersection_with(Rect(10, 10, 10, 10)) == Rect());
static_assert(Rect(25, 25, 450, 450).intersection_with(Rect(50, 50, 300, 300)) == Rect(50, 50, 300, 300));
static_assert(Rect(40, 60, 10, 100).intersection_with(Rect(25, 25, 450, 450)) == Rect(40, 60, 10, 100));
