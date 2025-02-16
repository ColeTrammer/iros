#pragma once

#include "di/container/string/prelude.h"
#include "di/reflect/prelude.h"
#include "di/types/integers.h"
#include "di/vocab/optional/prelude.h"
#include "di/vocab/span/prelude.h"
#include "ttx/params.h"

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

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<Color>) {
        return di::make_fields<"Color">(di::field<"c", &Color::c>, di::field<"r", &Color::r>, di::field<"g", &Color::g>,
                                        di::field<"b", &Color::b>);
    }
};

constexpr auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<Color::Palette>) {
    using enum Color::Palette;
    return di::make_enumerators<"Color::Palette">(
        di::enumerator<"None", None>, di::enumerator<"Custom", Custom>, di::enumerator<"Black", Black>,
        di::enumerator<"Red", Red>, di::enumerator<"Green", Green>, di::enumerator<"Brown", Brown>,
        di::enumerator<"Blue", Blue>, di::enumerator<"Magenta", Magenta>, di::enumerator<"Cyan", Cyan>,
        di::enumerator<"LightGrey", LightGrey>, di::enumerator<"DarkGrey", DarkGrey>,
        di::enumerator<"LightRed", LightRed>, di::enumerator<"LightGreen", LightGreen>,
        di::enumerator<"Yellow", Yellow>, di::enumerator<"LightBlue", LightBlue>,
        di::enumerator<"LightMagenta", LightMagenta>, di::enumerator<"LightCyan", LightCyan>,
        di::enumerator<"White", White>);
}

enum class BlinkMode : u8 {
    None,
    Normal,
    Rapid,
};

constexpr auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<BlinkMode>) {
    using enum BlinkMode;
    return di::make_enumerators<"BlinkMode">(di::enumerator<"None", None>, di::enumerator<"Normal", Normal>,
                                             di::enumerator<"Rapid", Rapid>);
}

enum class FontWeight : u8 {
    None,
    Bold,
    Dim,
};

constexpr auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<FontWeight>) {
    using enum FontWeight;
    return di::make_enumerators<"FontWeight">(di::enumerator<"None", None>, di::enumerator<"Bold", Bold>,
                                              di::enumerator<"Dim", Dim>);
}

enum class UnderlineMode : u8 {
    None = 0,
    Normal = 1,
    Double = 2,
    Curly = 3,
    Dotted = 4,
    Dashed = 5,
};

constexpr auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<UnderlineMode>) {
    using enum UnderlineMode;
    return di::make_enumerators<"UnderlineMode">(di::enumerator<"None", None>, di::enumerator<"Normal", Normal>,
                                                 di::enumerator<"Double", Double>, di::enumerator<"Curly", Curly>,
                                                 di::enumerator<"Dotted", Dotted>, di::enumerator<"Dashed", Dashed>);
}

struct GraphicsRendition {
    Color fg {};
    Color bg {};
    Color underline_color {};

    FontWeight font_weight { FontWeight::None };
    BlinkMode blink_mode { BlinkMode::None };
    UnderlineMode underline_mode { UnderlineMode::None };
    bool italic { false };
    bool overline { false };
    bool inverted { false };
    bool invisible { false };
    bool strike_through { false };

    static auto from_csi_params(Params const& params) {
        auto result = GraphicsRendition {};
        result.update_with_csi_params(params);
        return result;
    }
    void update_with_csi_params(Params const& params);
    auto as_csi_params() const -> di::Vector<Params>;

    auto operator==(GraphicsRendition const& other) const -> bool = default;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<GraphicsRendition>) {
        return di::make_fields<"GraphicsRendition">(
            di::field<"fg", &GraphicsRendition::fg>, di::field<"bg", &GraphicsRendition::bg>,
            di::field<"underline_color", &GraphicsRendition::underline_color>,
            di::field<"font_weight", &GraphicsRendition::font_weight>,
            di::field<"blink_mode", &GraphicsRendition::blink_mode>,
            di::field<"underline_mode", &GraphicsRendition::underline_mode>,
            di::field<"italic", &GraphicsRendition::italic>, di::field<"overline", &GraphicsRendition::overline>,
            di::field<"inverted", &GraphicsRendition::inverted>, di::field<"invisible", &GraphicsRendition::invisible>,
            di::field<"strike_through", &GraphicsRendition::strike_through>);
    }
};
}
