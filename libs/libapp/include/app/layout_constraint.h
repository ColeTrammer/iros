#pragma once

namespace App {
class LayoutConstraint {
public:
    static constexpr int AutoSize = -1;

    constexpr LayoutConstraint() {}
    constexpr LayoutConstraint(int width, int height) : m_width(width), m_height(height) {}

    constexpr int width() const { return m_width; }
    constexpr int height() const { return m_height; }

private:
    int m_width { AutoSize };
    int m_height { AutoSize };
};
}
