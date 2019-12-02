#pragma once

class Point {
public:
    Point() {
    }

    Point(int x, int y)
        : m_x(x)
        , m_y(y) {
    }

    ~Point() {
    }

    int x() const {
        return m_x;
    }
    int y() const {
        return m_y;
    }

    void set_x(int x) {
        m_x = x;
    }
    void set_y(int y) {
        m_y = y;
    }

private:
    int m_x { 0 };
    int m_y { 0 };
};