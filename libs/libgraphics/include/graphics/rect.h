#pragma once

#include <graphics/point.h>

class Rect {
public:
    Rect() {}

    Rect(int x, int y, int width, int height) : m_x(x), m_y(y), m_width(width), m_height(height) {}

    Rect(const Rect& other) : m_x(other.x()), m_y(other.y()), m_width(other.width()), m_height(other.height()) {}

    int x() const { return m_x; }
    int y() const { return m_y; }
    int width() const { return m_width; }
    int height() const { return m_height; }

    void set_x(int x) { m_x = x; }
    void set_y(int y) { m_y = y; }
    void set_width(int width) { m_width = width; }
    void set_height(int height) { m_height = height; }

    Point center() const { return Point(x() + width() / 2, y() + height() / 2); }

    bool intersects(Point p) const { return p.x() >= m_x && p.x() <= m_x + m_width && p.y() >= m_y && p.y() <= m_y + m_height; }

    ~Rect() {}

private:
    int m_x { 0 };
    int m_y { 0 };
    int m_width { 0 };
    int m_height { 0 };
};