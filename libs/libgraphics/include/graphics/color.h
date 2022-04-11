#pragma once

#include <liim/maybe.h>
#include <liim/string_view.h>
#include <stdint.h>

#include <kernel/hal/x86/drivers/vga.h>

enum class ColorValue {
    White,
    Black,
    DarkGray,
    LightGray,
    Clear,
};

class Color {
public:
    static Maybe<Color> parse(const StringView& view);

    constexpr Color() : Color(0xFF, 0xFF, 0xFF) {}
    constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xFF) { set(r, g, b, a); }
    constexpr Color(uint32_t value) : m_color(value) {}
    Color(enum vga_color color);

    constexpr Color(ColorValue v) {
        switch (v) {
            case ColorValue::White:
                set(255, 255, 255);
                break;
            case ColorValue::Black:
                set(0, 0, 0);
                break;
            case ColorValue::DarkGray:
                set(33, 33, 33);
                break;
            case ColorValue::LightGray:
                set(170, 170, 170);
                break;
            case ColorValue::Clear:
                set(0, 0, 0, 0);
                break;
        }
    }

    constexpr Color invert() const { return Color(255 - r(), 255 - g(), 255 - b(), a()); }

    constexpr uint32_t color() const { return m_color; }

    constexpr void set_alpha(uint8_t a) { set(r(), g(), b(), a); }
    constexpr void set(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xFF) { m_color = a << 24 | r << 16 | g << 8 | b; }

    constexpr uint8_t a() const { return (m_color & 0xFF000000U) >> 24; }
    constexpr uint8_t r() const { return (m_color & 0x00FF0000U) >> 16; }
    constexpr uint8_t g() const { return (m_color & 0x0000FF00U) >> 8; }
    constexpr uint8_t b() const { return (m_color & 0x000000FFU); }

    Maybe<vga_color> to_vga_color() const;

    constexpr bool is(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xFF) const {
        return this->r() == r && this->g() == g && this->b() == b && this->a() == a;
    }

    constexpr bool operator==(const Color& other) const { return this->color() == other.color(); }
    constexpr bool operator!=(const Color& other) const { return !(*this == other); }

private:
    uint32_t m_color { 0 };
};
