#pragma once

#include "di/reflect/prelude.h"

namespace ttx {
enum class CursorStyle {
    BlinkingBlock = 1,
    SteadyBlock = 2,
    BlinkingUnderline = 3,
    SteadyUnderline = 4,
    BlinkingBar = 5,
    SteadyBar = 6,
    Max,
};

constexpr auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<CursorStyle>) {
    using enum CursorStyle;
    return di::make_enumerators(
        di::enumerator<"BlinkingBlock", BlinkingBlock>, di::enumerator<"SteadyBlock", SteadyBlock>,
        di::enumerator<"BlinkingUnderline", BlinkingUnderline>, di::enumerator<"SteadyUnderline", SteadyUnderline>,
        di::enumerator<"BlinkingBar", BlinkingBar>, di::enumerator<"SteadyBar", SteadyBar>);
}
}
