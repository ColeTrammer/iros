#include <graphics/color.h>

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
