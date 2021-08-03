#pragma once

#include <graphics/color.h>
#include <liim/maybe.h>

namespace TInput {
struct TerminalTextStyle {
    Maybe<Color> foreground;
    Maybe<Color> background;
    bool bold { false };
    bool invert { false };

    bool operator==(const TerminalTextStyle& other) const {
        return this->foreground == other.foreground && this->background == other.background && this->bold == other.bold &&
               this->invert == other.invert;
    }
    bool operator!=(const TerminalTextStyle& other) const { return !(*this == other); }
};
}
