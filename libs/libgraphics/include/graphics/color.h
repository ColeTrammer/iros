#pragma once

#include <stdint.h>

class Color {
public:
    Color()
        : Color(0xFF, 0xFF, 0xFF)
    {
    }

    Color(uint8_t r, uint8_t b, uint8_t g, uint8_t a = 0x00)
        : m_color(a << 24 | r << 16 | g << 8 | b)
    {
    }

    uint32_t color() const { return m_color; }

private:
    uint32_t m_color;
};