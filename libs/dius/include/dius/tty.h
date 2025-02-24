#pragma once

#include "di/reflect/prelude.h"
#include "di/types/integers.h"

namespace dius::tty {
struct WindowSize {
    u32 rows { 0 };
    u32 cols { 0 };
    u32 pixel_width { 0 };
    u32 pixel_height { 0 };

    auto operator==(WindowSize const&) const -> bool = default;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<WindowSize>) {
        return di::make_fields<"WindowSize">(di::field<"rows", &WindowSize::rows>, di::field<"cols", &WindowSize::cols>,
                                             di::field<"pixel_width", &WindowSize::pixel_width>,
                                             di::field<"pixel_height", &WindowSize::pixel_height>);
    }
};
}
