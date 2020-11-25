#include <ctype.h>
#include <graphics/color.h>

Maybe<Color> Color::parse(const StringView& view) {
    if (view.size() != 7) {
        return {};
    }

    if (view[0] != '#' || !isxdigit(view[1]) || !isxdigit(view[2]) || !isxdigit(view[3]) || !isxdigit(view[4]) || !isxdigit(view[5]) ||
        !isxdigit(view[6])) {
        return {};
    }

    auto v = [](char c) -> uint8_t {
        if (isdigit(c)) {
            return c - '0';
        }
        return 10 + _tolower(c) - 'a';
    };

    uint8_t r = 16 * v(view[1]) + v(view[2]);
    uint8_t g = 16 * v(view[3]) + v(view[4]);
    uint8_t b = 16 * v(view[5]) + v(view[6]);
    return Color(r, g, b);
}

Color::Color(enum vga_color color) {
    switch (color) {
        case VGA_COLOR_BLACK:
            set(0, 0, 0);
            break;
        case VGA_COLOR_RED:
            set(170, 0, 0);
            break;
        case VGA_COLOR_GREEN:
            set(0, 170, 0);
            break;
        case VGA_COLOR_BROWN:
            set(170, 85, 0);
            break;
        case VGA_COLOR_BLUE:
            set(0, 0, 170);
            break;
        case VGA_COLOR_MAGENTA:
            set(170, 0, 170);
            break;
        case VGA_COLOR_CYAN:
            set(0, 170, 170);
            break;
        case VGA_COLOR_LIGHT_GREY:
            set(170, 170, 170);
            break;
        case VGA_COLOR_DARK_GREY:
            set(85, 85, 85);
            break;
        case VGA_COLOR_LIGHT_RED:
            set(255, 85, 85);
            break;
        case VGA_COLOR_LIGHT_GREEN:
            set(85, 255, 85);
            break;
        case VGA_COLOR_YELLOW:
            set(255, 255, 85);
            break;
        case VGA_COLOR_LIGHT_BLUE:
            set(85, 85, 255);
            break;
        case VGA_COLOR_LIGHT_MAGENTA:
            set(255, 85, 255);
            break;
        case VGA_COLOR_LIGHT_CYAN:
            set(85, 255, 255);
            break;
        case VGA_COLOR_WHITE:
            set(255, 255, 255);
            break;
        default:
            set(255, 255, 255);
            break;
    }
}

Maybe<vga_color> Color::to_vga_color() const {
    if (is(0, 0, 0)) {
        return VGA_COLOR_BLACK;
    }
    if (is(170, 0, 0)) {
        return VGA_COLOR_RED;
    }
    if (is(0, 170, 0)) {
        return VGA_COLOR_GREEN;
    }
    if (is(170, 85, 0)) {
        return VGA_COLOR_BROWN;
    }
    if (is(0, 0, 170)) {
        return VGA_COLOR_BLUE;
    }
    if (is(170, 0, 170)) {
        return VGA_COLOR_MAGENTA;
    }
    if (is(0, 170, 170)) {
        return VGA_COLOR_CYAN;
    }
    if (is(170, 170, 170)) {
        return VGA_COLOR_LIGHT_GREY;
    }
    if (is(85, 85, 85)) {
        return VGA_COLOR_DARK_GREY;
    }
    if (is(255, 85, 85)) {
        return VGA_COLOR_LIGHT_RED;
    }
    if (is(85, 255, 85)) {
        return VGA_COLOR_LIGHT_GREEN;
    }
    if (is(255, 255, 85)) {
        return VGA_COLOR_YELLOW;
    }
    if (is(85, 85, 255)) {
        return VGA_COLOR_LIGHT_BLUE;
    }
    if (is(255, 85, 255)) {
        return VGA_COLOR_LIGHT_MAGENTA;
    }
    if (is(85, 255, 255)) {
        return VGA_COLOR_LIGHT_CYAN;
    }
    if (is(255, 255, 255)) {
        return VGA_COLOR_WHITE;
    }
    return {};
}
