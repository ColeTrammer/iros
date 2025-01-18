#include "ttx/graphics_rendition.h"

#include "di/container/vector/vector.h"
#include "di/parser/integral.h"
#include "di/util/clamp.h"
#include "di/vocab/tuple/prelude.h"
#include "ttx/params.h"

namespace ttx {
// Parse complex color. This routine handles the following colors in the following form:
//   38;2;R;G;B   -- legacy form used for backwards compatability.
//   38:2:R:G:B   -- normal form with subparameters but without color space.
//   38:2:X:R:G:B -- subparameters with color space argument, which is ignored.
//   38:5:I       -- index form, specifices an index into color palette. For now, only 16 colors are supported.
// This returns both the number of parameters consumed as well as the parsed color.
static di::Tuple<usize, Color> parse_complex_color(Params const& params, usize start_index) {
    auto subparams = params.subparams(start_index);
    if (subparams.size() == 1) {
        // If there are no subparameters, use the legacy form.
        if (params.size() - start_index < 5 || params.get(start_index + 1) != 2) {
            return { 1, {} };
        }
        return { 5, Color(params.get(start_index + 2), params.get(start_index + 3), params.get(start_index + 4)) };
    }

    // Regular form with subparams
    switch (subparams.get(1)) {
        case 2:
            if (subparams.size() != 5 && subparams.size() != 6) {
                break;
            }
            return { 1, Color(subparams.get(subparams.size() - 3), subparams.get(subparams.size() - 2),
                              subparams.get(subparams.size() - 1)) };
        case 5:
            return { 1, Color(Color::Palette(Color::Palette::Black + subparams.get(2))) };
    }
    return { 1, {} };
}

// Parse any color specifer. This handles all cases for fg, bg, and underline_color.
static di::Tuple<usize, Color> parse_color(Params const& params, usize start_index) {
    auto command = params.get(start_index);

    // Complex colors
    if (command == 38 || command == 48 || command == 58) {
        return parse_complex_color(params, start_index);
    }

    // High colors
    auto palette_index = command % 10;
    if (command >= 90) {
        return { 1, Color(Color::Palette(Color::Palette::DarkGrey + palette_index)) };
    }

    // Reset
    if (palette_index == 9) {
        return { 1, {} };
    }

    // Palette color
    return { 1, Color(Color::Palette(Color::Palette::Black + palette_index)) };
}

// Select Graphics Rendition - https://vt100.net/docs/vt510-rm/SGR.html
//   Modern extensions like underline and true color can be found here:
//     https://wezfurlong.org/wezterm/escape-sequences.html#graphic-rendition-sgr
void GraphicsRendition::update_with_csi_params(Params const& params) {
    // No params = reset.
    if (params.empty()) {
        *this = {};
        return;
    }

    for (auto i = 0_usize; i == 0 || i < params.size(); i++) {
        switch (params.get(i, 0)) {
            case 0:
                *this = {};
                break;
            case 1:
                font_weight = FontWeight::Bold;
                break;
            case 2:
                font_weight = FontWeight::Dim;
                break;
            case 3:
                italic = true;
                break;
            case 4: {
                auto subparam = params.get_subparam(i, 1, 1);
                switch (subparam) {
                    case 0:
                        underline_mode = UnderlineMode::None;
                        break;
                    case 1:
                        underline_mode = UnderlineMode::Normal;
                        break;
                    case 2:
                        underline_mode = UnderlineMode::Double;
                        break;
                    case 3:
                        underline_mode = UnderlineMode::Curly;
                        break;
                    case 4:
                        underline_mode = UnderlineMode::Dotted;
                        break;
                    case 5:
                        underline_mode = UnderlineMode::Dashed;
                        break;
                }
                break;
            }
            case 5:
                blink_mode = BlinkMode::Normal;
                break;
            case 6:
                blink_mode = BlinkMode::Rapid;
                break;
            case 7:
                inverted = true;
                break;
            case 8:
                invisible = true;
                break;
            case 9:
                strike_through = true;
                break;
            case 21:
                underline_mode = UnderlineMode::Double;
                break;
            case 22:
                font_weight = FontWeight::None;
                break;
            case 23:
                italic = false;
                break;
            case 24:
                underline_mode = UnderlineMode::None;
                break;
            case 25:
                blink_mode = BlinkMode::None;
                break;
            case 27:
                inverted = false;
                break;
            case 28:
                invisible = false;
                break;
            case 29:
                strike_through = false;
                break;
            case 30:
            case 31:
            case 32:
            case 33:
            case 34:
            case 35:
            case 36:
            case 37:
            case 38:
            case 39:
            case 90:
            case 91:
            case 92:
            case 93:
            case 94:
            case 95:
            case 96:
            case 97: {
                auto [n, c] = parse_color(params, i);
                i += n - 1;
                fg = c;
                break;
            }
            case 40:
            case 41:
            case 42:
            case 43:
            case 44:
            case 45:
            case 46:
            case 47:
            case 48:
            case 49:
            case 100:
            case 101:
            case 102:
            case 103:
            case 104:
            case 105:
            case 106:
            case 107: {
                auto [n, c] = parse_color(params, i);
                i += n - 1;
                bg = c;
                break;
            }
            case 53:
                overline = true;
                break;
            case 55:
                overline = false;
                break;
            case 58:
            case 59: {
                auto [n, c] = parse_color(params, i);
                i += n - 1;
                underline_color = c;
                break;
            }
            default:
                break;
        }
    }
}

enum class ColorType {
    Fg,
    Bg,
    Underine,
};

static auto color_to_subparams(Color c, ColorType type) -> di::Vector<u32> {
    if (c.c == Color::Palette::Custom) {
        auto code = type == ColorType::Fg ? 38u : type == ColorType::Bg ? 48u : 58u;
        return { code, 2, c.r, c.g, c.b };
    } else if (type == ColorType::Underine) {
        // Use palette index.
        return { 58, 5, u32(c.c - Color::Palette::Black) };
    } else if (c.c <= Color::Palette::LightGrey) {
        auto base_value = type == ColorType::Fg ? 30u : 40u;
        return { base_value + c.c - Color::Palette::Black };
    } else {
        auto base_value = type == ColorType::Fg ? 90u : 100u;
        return { base_value + c.c - Color::Palette::DarkGrey };
    }
}

auto GraphicsRendition::as_csi_params() const -> Params {
    // Start by clearing all attributes.
    auto sgr = Params();
    sgr.add_param(0);

    switch (font_weight) {
        case FontWeight::Bold:
            sgr.add_param(1);
            break;
        case FontWeight::Dim:
            sgr.add_param(2);
            break;
        case FontWeight::None:
            break;
    }
    if (italic) {
        sgr.add_param(3);
    }
    switch (underline_mode) {
        case UnderlineMode::Normal:
            sgr.add_param(4);
            break;
        case UnderlineMode::Double:
            sgr.add_param(21);
            break;
        case UnderlineMode::Curly:
            sgr.add_subparams({ 4, 3 });
            break;
        case UnderlineMode::Dotted:
            sgr.add_subparams({ 4, 4 });
            break;
        case UnderlineMode::Dashed:
            sgr.add_subparams({ 4, 5 });
            break;
        case UnderlineMode::None:
            break;
    }
    switch (blink_mode) {
        case BlinkMode::Normal:
            sgr.add_param(5);
            break;
        case BlinkMode::Rapid:
            sgr.add_param(6);
            break;
        case BlinkMode::None:
            break;
    }
    if (inverted) {
        sgr.add_param(7);
    }
    if (invisible) {
        sgr.add_param(8);
    }
    if (strike_through) {
        sgr.add_param(9);
    }
    if (overline) {
        sgr.add_param(53);
    }
    if (fg.c != Color::None) {
        sgr.add_subparams(color_to_subparams(fg, ColorType::Fg));
    }
    if (bg.c != Color::None) {
        sgr.add_subparams(color_to_subparams(bg, ColorType::Bg));
    }
    if (underline_color.c != Color::None) {
        sgr.add_subparams(color_to_subparams(underline_color, ColorType::Underine));
    }
    return sgr;
}
}
