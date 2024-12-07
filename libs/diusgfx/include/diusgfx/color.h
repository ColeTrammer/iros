#pragma once

#include "di/types/integers.h"
#include "di/types/strong_ordering.h"

namespace gfx {
struct RGBA32 {
    u8 r;
    u8 b;
    u8 g;
    u8 a;
};

class Color {
public:
    Color() = default;
    constexpr explicit Color(u8 r, u8 g, u8 b, u8 a = 255) : m_red(r), m_green(g), m_blue(b), m_alpha(a) {}

    constexpr auto red() const -> u8 { return m_red; }
    constexpr auto green() const -> u8 { return m_green; }
    constexpr auto blue() const -> u8 { return m_blue; }
    constexpr auto alpha() const -> u8 { return m_alpha; }

    constexpr auto with_red(u8 red) const -> Color { return Color(red, green(), blue(), alpha()); }
    constexpr auto with_green(u8 green) const -> Color { return Color(red(), green, blue(), alpha()); }
    constexpr auto with_blue(u8 blue) const -> Color { return Color(red(), green(), blue, alpha()); }
    constexpr auto with_alpha(u8 alpha) const -> Color { return Color(red(), green(), blue(), alpha); }

    constexpr auto as_rgba32() const -> RGBA32 { return { red(), blue(), green(), alpha() }; }

    auto operator==(Color const&) const -> bool = default;
    auto operator<=>(Color const&) const = default;

private:
    u8 m_red { 0 };
    u8 m_green { 0 };
    u8 m_blue { 0 };
    u8 m_alpha { 255 };
};
}
