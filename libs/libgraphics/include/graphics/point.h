#pragma once

class Point {
public:
    constexpr Point() {}
    constexpr Point(int x, int y) : m_x(x), m_y(y) {}

    constexpr int x() const { return m_x; }
    constexpr int y() const { return m_y; }

    constexpr void set_x(int x) { m_x = x; }
    constexpr void set_y(int y) { m_y = y; }

    constexpr Point translated(int d) const { return translated(d, d); }
    constexpr Point translated(int dx, int dy) const { return { x() + dx, y() + dy }; }
    constexpr Point translated(const Point& p) const { return translated(p.x(), p.y()); }

    constexpr Point with_x(int x) const { return { x, y() }; }
    constexpr Point with_y(int y) const { return { x(), y }; }

    constexpr bool operator==(const Point& other) const { return this->x() == other.x() && this->y() == other.y(); }
    constexpr bool operator!=(const Point& other) const { return !(*this == other); }

private:
    int m_x { 0 };
    int m_y { 0 };
};
