#pragma once

#include <graphics/color.h>
#include <liim/option.h>

namespace TInput {
struct TerminalTextStyle {
    Option<Color> foreground;
    Option<Color> background;
    bool bold { false };
    bool invert { false };

    bool operator==(const TerminalTextStyle& other) const {
        return this->foreground == other.foreground && this->background == other.background && this->bold == other.bold &&
               this->invert == other.invert;
    }
    bool operator!=(const TerminalTextStyle& other) const { return !(*this == other); }
};
}
