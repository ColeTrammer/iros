#pragma once

class Point {
public:
    constexpr Point() {}
    constexpr Point(int x, int y) : m_x(x), m_y(y) {}

    constexpr int x() const { return m_x; }
    constexpr int y() const { return m_y; }

    constexpr void set_x(int x) { m_x = x; }
    constexpr void set_y(int y) { m_y = y; }

    constexpr bool operator==(const Point& other) const { return this->x() == other.x() && this->y() == other.y(); }
    constexpr bool operator!=(const Point& other) const { return !(*this == other); }

private:
    int m_x { 0 };
    int m_y { 0 };
};
