#pragma once

#include "di/reflect/enumerator.h"
#include "di/reflect/reflect.h"
#include "di/util/bitwise_enum.h"
#include "dius/tty.h"

// Mouse reference: https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h2-Mouse-Tracking
namespace ttx {
enum class MouseButton {
    None = 0,
    Left = 1 << 0,
    Middle = 1 << 1,
    Right = 1 << 2,
    ScrollUp = 1 << 3,
    ScrollDown = 1 << 4,
    ScrollLeft = 1 << 5,
    ScrollRight = 1 << 6,
    _8 = 1 << 7,
    _9 = 1 << 8,
    _10 = 1 << 9,
    _11 = 1 << 10,
    HorizontalScrollButtons = ScrollRight | ScrollLeft,
    VerticalScrollButtons = ScrollUp | ScrollDown,
    ScrollButtons = HorizontalScrollButtons | VerticalScrollButtons,
};

DI_DEFINE_ENUM_BITWISE_OPERATIONS(MouseButton)

constexpr auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<MouseButton>) {
    using enum MouseButton;
    return di::make_enumerators<"MouseButton">(
        di::enumerator<"None", None>, di::enumerator<"Left", Left>, di::enumerator<"Middle", Middle>,
        di::enumerator<"Right", Right>, di::enumerator<"ScrollUp", ScrollUp>, di::enumerator<"ScrollDown", ScrollDown>,
        di::enumerator<"ScrollLeft", ScrollLeft>, di::enumerator<"ScrollRight", ScrollRight>, di::enumerator<"8", _8>,
        di::enumerator<"9", _9>, di::enumerator<"10", _10>, di::enumerator<"11", _11>);
}

class MouseCoordinate {
public:
    MouseCoordinate() = default;
    constexpr MouseCoordinate(u32 x, u32 y) : m_x(x), m_y(y) {}

    constexpr auto x() const -> u32 { return m_x; }
    constexpr auto y() const -> u32 { return m_y; }

    auto operator==(MouseCoordinate const&) const -> bool = default;

private:
    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<MouseCoordinate>) {
        return di::make_fields<"MouseCoordinate">(di::field<"x", &MouseCoordinate::m_x>,
                                                  di::field<"y", &MouseCoordinate::m_y>);
    }

    u32 m_x { 0 };
    u32 m_y { 0 };
};

class MousePosition {
public:
    constexpr static auto from_pixels(MouseCoordinate pixels, dius::tty::WindowSize const& size) -> MousePosition {
        if (size.rows == 0 || size.cols == 0) {
            return MousePosition({}, pixels);
        }

        // Determine the cell from pixel coordinates.
        auto in_cells =
            MouseCoordinate(pixels.x() * size.cols / size.pixel_width, pixels.y() * size.rows / size.pixel_height);
        return MousePosition(in_cells, pixels);
    }

    MousePosition() = default;
    constexpr explicit MousePosition(MouseCoordinate cells, di::Optional<MouseCoordinate> pixels = {})
        : m_cells(cells), m_pixels(pixels) {}

    constexpr auto in_cells() const -> MouseCoordinate { return m_cells; }
    constexpr auto in_pixels() const -> di::Optional<MouseCoordinate> { return m_pixels; }

    constexpr auto in_pixels_with_fallback(dius::tty::WindowSize const& size) const -> MouseCoordinate {
        if (m_pixels.has_value()) {
            return m_pixels.value();
        }

        if (size.rows == 0 || size.cols == 0) {
            return {};
        }

        // Infer the pixel coordinates to be in the middle of the cell.
        auto x = (in_cells().x() * size.pixel_width + size.pixel_width / 2) / size.cols;
        auto y = (in_cells().y() * size.pixel_height + size.pixel_height / 2) / size.rows;
        return { x, y };
    }

    constexpr auto translate(MouseCoordinate offset_in_cells, dius::tty::WindowSize const& size) const
        -> MousePosition {
        auto new_cells = MouseCoordinate {
            in_cells().x() + offset_in_cells.x(),
            in_cells().y() + offset_in_cells.y(),
        };
        auto new_pixels = in_pixels().transform([&](MouseCoordinate pixels) {
            return MouseCoordinate {
                pixels.x() + size.pixel_width / size.cols,
                pixels.y() + size.pixel_height / size.rows,
            };
        });

        return MousePosition(new_cells, new_pixels);
    }

    auto operator==(MousePosition const&) const -> bool = default;

private:
    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<MousePosition>) {
        return di::make_fields<"MousePosition">(di::field<"cells", &MousePosition::m_cells>,
                                                di::field<"pixels", &MousePosition::m_pixels>);
    }

    MouseCoordinate m_cells;
    di::Optional<MouseCoordinate> m_pixels;
};
}
