#include "ttx/mouse_event_io.h"

#include "di/container/string/encoding.h"
#include "di/format/prelude.h"
#include "di/util/unreachable.h"
#include "dius/tty.h"
#include "ttx/key_event.h"
#include "ttx/key_event_io.h"
#include "ttx/modifiers.h"
#include "ttx/mouse.h"
#include "ttx/mouse_event.h"

namespace ttx {
struct ButtonMapping {
    u32 number { 0 };
    MouseButton button { MouseButton::None };
};

constexpr auto button_mappings = di::to_array<ButtonMapping>({
    { 0, MouseButton::Left },
    { 1, MouseButton::Middle },
    { 2, MouseButton::Right },
    { 3, MouseButton::None },
    { 64, MouseButton::ScrollUp },
    { 65, MouseButton::ScrollDown },
    { 66, MouseButton::ScrollLeft },
    { 67, MouseButton::ScrollRight },
    { 128, MouseButton::_8 },
    { 129, MouseButton::_9 },
    { 130, MouseButton::_10 },
    { 131, MouseButton::_11 },
});

static auto mouse_button_to_number(MouseButton target) -> u32 {
    for (auto [number, button] : button_mappings) {
        if (target == button) {
            return number;
        }
    }

    di::unreachable();
    return 0;
}

static auto modifiers_to_number(Modifiers modifiers) -> u32 {
    auto result = 0_u32;
    if (!!(modifiers & Modifiers::Shift)) {
        result += 4;
    }
    if (!!(modifiers & Modifiers::Alt)) {
        result += 8;
    }
    if (!!(modifiers & Modifiers::Control)) {
        result += 16;
    }
    return result;
}

static auto mouse_number(MouseEventType type, MouseButton button, Modifiers modifiers, bool default_release_events) {
    auto number = mouse_button_to_number(button) + modifiers_to_number(modifiers);
    if (type == MouseEventType::Move) {
        number += 32;
    } else if (default_release_events && type == MouseEventType::Release) {
        number = 3;
    }
    return number;
}

static auto serialize_as_x10(MouseEventType type, MouseButton button, MouseCoordinate position, Modifiers modifiers)
    -> di::Optional<di::TransparentString> {
    // NOTE: since the position is encoded in a single byte the range of representable
    // coordinates are limited.
    if (position.x() + 33 > 255 || position.y() + 33 > 255) {
        return {};
    }

    auto number = mouse_number(type, button, modifiers, true);

    // Return CSI M CbCxCy
    auto result = ""_ts;
    result.append("\033[M"_tsv);
    result.push_back(char(number + 32));
    result.push_back(char(position.x() + 33));
    result.push_back(char(position.y() + 33));
    return result;
}

static auto serialize_as_utf8(MouseEventType type, MouseButton button, MouseCoordinate position, Modifiers modifiers)
    -> di::Optional<di::TransparentString> {
    // We can output any value up to the maximum UTF-8 code point. Note that only the coordinates are encoded
    // in UTF-8, not the button itself, so the resulting string is not guaranteed to be valid UTF-8.
    if (position.x() + 33 > 0x10FFFF || position.y() + 33 > 0x10FFFF) {
        return {};
    }

    auto number = mouse_number(type, button, modifiers, true);

    // Return CSI M CbCxCy
    auto result = ""_ts;
    result.append("\033[M"_tsv);
    result.push_back(char(number + 32));

    for (auto byte :
         di::container::string::encoding::convert_to_code_units(di::String::Encoding {}, position.x() + 33)) {
        result.push_back(char(byte));
    }
    for (auto byte :
         di::container::string::encoding::convert_to_code_units(di::String::Encoding {}, position.y() + 33)) {
        result.push_back(char(byte));
    }
    return result;
}

static auto serialize_as_urxvt(MouseEventType type, MouseButton button, MouseCoordinate position, Modifiers modifiers)
    -> di::Optional<di::TransparentString> {
    auto number = mouse_number(type, button, modifiers, true);

    // Return CSI Cb;Cx;Cy M
    return *di::present("\033[{};{};{}M"_sv, number + 32, position.x() + 1, position.y() + 1) |
           di::transform([](c32 value) {
               return char(value);
           }) |
           di::to<di::TransparentString>();
}

static auto serialize_as_sgr(MouseEventType type, MouseButton button, MouseCoordinate position, Modifiers modifiers)
    -> di::Optional<di::TransparentString> {
    auto number = mouse_number(type, button, modifiers, false);

    // Return CSI < Cb;Cx;Cy [Mm]
    auto final_char = type == MouseEventType::Release ? U'm' : U'M';
    return *di::present("\033[<{};{};{}{}"_sv, number, position.x(), position.y(), final_char) |
           di::transform([](c32 value) {
               return char(value);
           }) |
           di::to<di::TransparentString>();
}

auto serialize_mouse_event(MouseEvent const& event, MouseProtocol protocol, MouseEncoding encoding,
                           di::Optional<MousePosition> prev_event_position, MouseScrollProtocol const& scroll_protocol,
                           dius::tty::WindowSize const& window_size) -> di::Optional<di::TransparentString> {
    // Check if mouse scroll protocol applies. This means the base mouse protocol woulnt't report the scroll event.
    if ((protocol == MouseProtocol::None || protocol == MouseProtocol::X10) && event.is_vertical_scroll() &&
        event.type() == MouseEventType::Press) {
        if (scroll_protocol.in_alternate_screen_buffer &&
            scroll_protocol.alternate_scroll_mode == AlternateScrollMode::Enabled) {
            auto key = event.button() == MouseButton::ScrollUp ? Key::Up : Key::Down;
            return serialize_key_event(KeyEvent::key_down(key), scroll_protocol.application_cursor_keys_mode,
                                       KeyReportingFlags::None)
                       .value() |
                   di::transform([](c32 value) {
                       return char(value);
                   }) |
                   di::to<di::TransparentString>();
        }
        return {};
    }

    auto reported_by_protocol = [&] -> bool {
        switch (protocol) {
            case MouseProtocol::None:
                return false;
            case MouseProtocol::X10:
                return event.type() == MouseEventType::Press &&
                       !!(event.button() & (MouseButton::Left | MouseButton::Middle | MouseButton::Right));
            case MouseProtocol::VT200:
                return event.type() != MouseEventType::Move;
            case MouseProtocol::BtnEvent:
                return event.type() != MouseEventType::Move || event.button() != MouseButton::None;
            case MouseProtocol::AnyEvent:
                return true;
        }
        return false;
    }();
    if (!reported_by_protocol) {
        return {};
    }

    auto type = event.type();
    auto button = event.button();
    auto modifiers = event.modifiers();
    auto position = event.position().in_cells();
    auto prev_position = prev_event_position.transform(&MousePosition::in_cells);

    // X10 mode ignores modifiers.
    if (protocol == MouseProtocol::X10) {
        modifiers = Modifiers::None;
    }

    // SGR pixel mode uses pixels as the position.
    if (encoding == MouseEncoding::SGRPixels) {
        position = event.position().in_pixels_with_fallback(window_size);
        prev_position = prev_event_position.transform([&](MousePosition const& position) {
            return position.in_pixels_with_fallback(window_size);
        });
    }

    // If a move event resulted in the position being unchanged, ignore.
    if (type == MouseEventType::Move && position == prev_position) {
        return {};
    }

    switch (encoding) {
        case MouseEncoding::X10:
            return serialize_as_x10(type, button, position, modifiers);
        case MouseEncoding::UTF8:
            return serialize_as_utf8(type, button, position, modifiers);
        case MouseEncoding::URXVT:
            return serialize_as_urxvt(type, button, position, modifiers);
        case MouseEncoding::SGR:
            return serialize_as_sgr(type, button, { position.x() + 1, position.y() + 1 }, modifiers);
        case MouseEncoding::SGRPixels:
            return serialize_as_sgr(type, button, position, modifiers);
    }

    return {};
}

auto mouse_event_from_csi(CSI const& csi, di::Optional<dius::tty::WindowSize> window_size_if_using_pixels)
    -> di::Optional<MouseEvent> {
    constexpr auto flags = u32(4 | 8 | 16 | 32);

    // For now, only the SGR format is supported. This looks like:
    //   CSI < Pb;Px;Py [Mm]

    if (csi.intermediate != "<"_sv) {
        return {};
    }

    auto const& params = csi.params;
    auto button_code = params.get(0);
    auto x = params.get(1, 1);
    auto y = params.get(2, 1);
    if (!window_size_if_using_pixels.has_value()) {
        // Prevent underflow if the terminal mistakenly sent 0 (the coordinates are 1 indexed).
        if (x != 0) {
            x--;
        }
        if (y != 0) {
            y--;
        }
    }

    auto modifiers = Modifiers::None;
    auto type = csi.terminator == u'M' ? MouseEventType::Press : MouseEventType::Release;

    if (button_code & 4) {
        modifiers |= Modifiers::Shift;
    }
    if (button_code & 8) {
        modifiers |= Modifiers::Alt;
    }
    if (button_code & 16) {
        modifiers |= Modifiers::Control;
    }
    if (!!(button_code & 32) && type == MouseEventType::Press) {
        type = MouseEventType::Move;
    }
    button_code &= ~flags;

    auto button = di::Optional<MouseButton>();
    for (auto const& [code, btn] : button_mappings) {
        if (code == button_code) {
            button = btn;
            break;
        }
    }
    if (!button.has_value()) {
        return {};
    }

    auto position = window_size_if_using_pixels.has_value()
                        ? MousePosition::from_pixels({ x, y }, window_size_if_using_pixels.value())
                        : MousePosition({ x, y });
    return MouseEvent(type, button.value(), position, modifiers);
}
}
