#pragma once

#include "di/reflect/prelude.h"
#include "di/types/integers.h"

namespace dius::tty {
struct WindowSize {
    u32 rows { 0 };
    u32 cols { 0 };
    u32 pixel_width { 0 };
    u32 pixel_height { 0 };

    auto rows_shrinked(u32 r) -> WindowSize {
        if (r >= rows) {
            return { 0, cols, pixel_width, 0 };
        }
        return { rows - r, cols, pixel_width, pixel_height - (r * pixel_height / rows) };
    }

    auto cols_shrinked(u32 c) -> WindowSize {
        if (c >= cols) {
            return { rows, 0, 0, pixel_height };
        }
        return { rows, cols - c, pixel_width - (c * pixel_width / cols), pixel_height };
    }

    auto operator==(WindowSize const&) const -> bool = default;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<WindowSize>) {
        return di::make_fields<"WindowSize">(di::field<"rows", &WindowSize::rows>, di::field<"cols", &WindowSize::cols>,
                                             di::field<"pixel_width", &WindowSize::pixel_width>,
                                             di::field<"pixel_height", &WindowSize::pixel_height>);
    }
};
}
