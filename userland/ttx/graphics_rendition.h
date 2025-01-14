#pragma once

#include "di/container/string/prelude.h"
#include "di/types/integers.h"
#include "di/vocab/optional/prelude.h"
#include "di/vocab/span/prelude.h"

namespace ttx {
struct Color {
    enum Palette : u8 {
        None,
        Custom,
        Black,
        Red,
        Green,
        Brown,
        Blue,
        Magenta,
        Cyan,
        LightGrey,
        DarkGrey,
        LightRed,
        LightGreen,
        Yellow,
        LightBlue,
        LightMagenta,
        LightCyan,
        White,
    };

    Color() = default;
    constexpr Color(Palette c) : c(c) {}
    constexpr Color(u8 r, u8 g, u8 b) : c(Palette::Custom), r(r), g(g), b(b) {}

    Palette c = Palette::None;
    u8 r = 0;
    u8 g = 0;
    u8 b = 0;

    auto operator==(Color const& other) const -> bool = default;
};

enum class BlinkMode : u8 {
    None,
    Normal,
    Rapid,
};

enum class FontWeight : u8 {
    None,
    Bold,
    Dim,
};

enum class UnderlineMode : u8 {
    None = 0,
    Normal = 1,
    Double = 2,
    Curly = 3,
    Dotted = 4,
    Dashed = 5,
};

struct GraphicsRendition {
    Color fg;
    Color bg;
    Color underline_color;

    FontWeight font_weight : 2 { FontWeight::None };
    BlinkMode blink_mode : 2 { BlinkMode::None };
    UnderlineMode underline_mode : 3 { UnderlineMode::None };
    bool italic : 1 { false };
    bool overline : 1 { false };
    bool inverted : 1 { false };
    bool invisible : 1 { false };
    bool strike_through : 1 { false };

    void update_with_csi_params(di::Span<i32 const> params);
    auto as_csi_params() const -> di::String;

    auto operator==(GraphicsRendition const& other) const -> bool = default;
};
}
